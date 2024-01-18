#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "stm32f4xx_hal.h"
#include <stdlib.h> 
#include "stdio.h"
#include "lcd.h"
#include "Driver_SPI.h"
#include "Arial12x12.h"
#include "lcd.h"

osThreadId_t lcd;                        // thread id
//osThreadId_t timer;
//osThreadId_t rebotes;
void lcd_Func(void *argument);                   // thread function
void timer_Func(void *argument);                   // thread function
void rebotes_Func(void *argument);                   // thread function
int primera = 0;

osMessageQueueId_t queue_lcd; 


extern ARM_DRIVER_SPI Driver_SPI1;
ARM_DRIVER_SPI* SPIdrv = &Driver_SPI1;

unsigned char buffer[512];
unsigned char buffer2[512];
int posicionL1 = 0, posicionL2 = 256, posicionLCD = 0;;
int flagL2 = 0, flagF2 = 0, full = 0;

void init_SPI(void){
	/* Initialize the SPI driver */
	SPIdrv->Initialize(NULL);
	/* Power up the SPI peripheral */
	SPIdrv->PowerControl(ARM_POWER_FULL);
	/* Configure the SPI to Master, 8-bit mode @10000 kBits/sec */
	SPIdrv->Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 | ARM_SPI_MSB_LSB | ARM_SPI_DATA_BITS(8), 20000000);
}

static void init_GPIO(void){
	GPIO_InitTypeDef GPIO_InitStruct; //Definicion los GPIO

	__HAL_RCC_GPIOA_CLK_ENABLE();	 //Habilitar el reloj de los puertos GPIO A
	__HAL_RCC_GPIOB_CLK_ENABLE();	 //Habilitar el reloj de los puertos GPIO B
	__HAL_RCC_GPIOD_CLK_ENABLE();	 //Habilitar el reloj de los puertos GPIO D
	__HAL_RCC_GPIOF_CLK_ENABLE();	 //Habilitar el reloj de los puertos GPIO F

	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; 
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

	GPIO_InitStruct.Pin = GPIO_PIN_14; //CS
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_13; //A0
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_6; //Reset
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	//LED 1,2 y 3
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_7 | GPIO_PIN_14;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void delay(uint32_t microsegundos)
{	
	uint32_t periodo = microsegundos - 1;
	TIM_HandleTypeDef htim7; //Definimos el TIM7
	// Configurar y arrancar el timer para generar un evento pasados n_microsegundos
	htim7.Instance = TIM7; 
	htim7.Init.Prescaler = 83; //Prescaler a 83, El reloj de APB1 es de 84 MHz / Prescaler (83) = 1MHz
	htim7.Init.Period = periodo; //Para obtener el tiempo dividimos periodo/frecuencia de conteo (en este caso 10KHz)
	__HAL_RCC_TIM7_CLK_ENABLE(); //Habilitar reloj del timer 7
	
	HAL_TIM_Base_Init(&htim7); //Configurar el timer
	HAL_TIM_Base_Start(&htim7); //Iniciar el timer
	
	// Esperar a que se active el flag del registro de desbordamiento (overflow)
	while ((htim7.Instance->SR & TIM_SR_UIF) == 0) {
	}
	// Borrar el flag de desbordamiento
	htim7.Instance->SR &= ~TIM_SR_UIF;

	// Parar el Timer y ponerlo a 0 para la siguiente llamada a la funci?n
	HAL_TIM_Base_Stop(&htim7);
	__HAL_TIM_SET_COUNTER(&htim7, 0);
}

void LCD_Reset(void){
	init_SPI(); //Inicializar SPI
	init_GPIO(); //Iniciar GPIO

	//Iniciar los pines en valor alto
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET); //Reset
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14,	GPIO_PIN_SET); //CS
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET); //A0

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	delay(10);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
}
void LCD_wr_data(unsigned char data)
{
 // Seleccionar CS = 0;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14,	GPIO_PIN_RESET); //CS

 // Seleccionar A0 = 1;
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET); //A0

 // Escribir un dato (data) usando la funci?n SPIDrv->Send(?);
	SPIdrv ->Send(&data, sizeof(data));
 // Esperar a que se libere el bus SPI;
	while(SPIdrv->GetStatus().busy == 1){

	}
 // Seleccionar CS = 1;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14,	GPIO_PIN_SET); //CS
}

void LCD_wr_cmd(unsigned char cmd)
{
 // Seleccionar CS = 0;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14,	GPIO_PIN_RESET); //CS
 // Seleccionar A0 = 0;
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13 , GPIO_PIN_RESET); //A0

 // Escribir un comando (cmd) usando la funci?n SPIDrv->Send(?);
	SPIdrv ->Send(&cmd, sizeof(cmd));
 // Esperar a que se libere el bus SPI;
	while(SPIdrv->GetStatus().busy == 1){

	}
 // Seleccionar CS = 1;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14,	GPIO_PIN_SET); //CS
}

void LCD_Init(void){
	LCD_Reset();
	LCD_wr_cmd(0xAE); //Display OFF
	LCD_wr_cmd(0xA2);
	LCD_wr_cmd(0xA0);
	LCD_wr_cmd(0xC8);
	LCD_wr_cmd(0x22);
	LCD_wr_cmd(0x2F); //Power On
	LCD_wr_cmd(0x40); //Display comienza en linea 0
	LCD_wr_cmd(0xAF); //Display ON
	LCD_wr_cmd(0x81); //Contraste
	LCD_wr_cmd(0x0B); //Valor Contraste
	LCD_wr_cmd(0xA4); //Desplegar Puntos normal, A5 activa todos los puntos
	LCD_wr_cmd(0xA6); //Display normal (Blanco), Display Inverso (Negro) A7
}

void LCD_clear(void)
{
	int i;
	
	for(i=0;i<512;i++){
		buffer[i] = 0x00;
	}
	
	LCD_wr_cmd(0x00); // 4 bits de la parte baja de la direcci?n a 0
	LCD_wr_cmd(0x10); // 4 bits de la parte alta de la direcci?n a 0
	LCD_wr_cmd(0xB0); // P?gina 0
	for(i=0;i<128;i++){
		LCD_wr_data(0x00);
	}

	LCD_wr_cmd(0x00); // 4 bits de la parte baja de la direcci?n a 0
	LCD_wr_cmd(0x10); // 4 bits de la parte alta de la direcci?n a 0
	LCD_wr_cmd(0xB1); // P?gina 1
	for(i=0;i<128;i++){
		LCD_wr_data(0x00);
	}

	LCD_wr_cmd(0x00);
	LCD_wr_cmd(0x10);
	LCD_wr_cmd(0xB2); //P?gina 2
	for(i=0;i<128;i++){
		LCD_wr_data(0x00);
	}

	LCD_wr_cmd(0x00);
	LCD_wr_cmd(0x10);
	LCD_wr_cmd(0xB3); // Pagina 3
	for(i=0;i<128;i++){
		LCD_wr_data(0x00);
	}
	posicionL1 = 0;
	posicionLCD = 0;
	posicionL2  = 256;
	full = 0;
}

void LCD_update(void)
{
	int i;
	LCD_wr_cmd(0x00); // 4 bits de la parte baja de la direcci?n a 0
	LCD_wr_cmd(0x10); // 4 bits de la parte alta de la direcci?n a 0
	LCD_wr_cmd(0xB0); // P?gina 0
	for(i = 0; i < 128; i++){
		LCD_wr_data(buffer[i]);
	}
	
	LCD_wr_cmd(0x00); // 4 bits de la parte baja de la direcci?n a 0
	LCD_wr_cmd(0x10); // 4 bits de la parte alta de la direcci?n a 0
	LCD_wr_cmd(0xB1); // P?gina 1
	for(i = 128; i < 256; i++){
		LCD_wr_data(buffer[i]);
	}
	
	LCD_wr_cmd(0x00); // 4 bits de la parte baja de la direcci?n a 0
	LCD_wr_cmd(0x10); // 4 bits de la parte alta de la direcci?n a 0
	LCD_wr_cmd(0xB2); // P?gina 2
	for(i = 256; i < 384; i++){
		LCD_wr_data(buffer[i]);
	}
	
	LCD_wr_cmd(0x00); // 4 bits de la parte baja de la direcci?n a 0
	LCD_wr_cmd(0x10); // 4 bits de la parte alta de la direcci?n a 0
	LCD_wr_cmd(0xB3); // P?gina 3
	for(i = 384; i < 512; i++){
		LCD_wr_data(buffer[i]);
	}
}

void symbolToLocalBuffer_L1(uint8_t symbol){
	uint8_t i, value1, value2;
	uint16_t offset = 0;
	
	offset = 25* (symbol - ' ');
	for (i = 0; i < Arial12x12[offset]; i++){
		if(i + posicionLCD > 127){
				posicionLCD += i;
				return;
		}
		value1 = Arial12x12[offset+i*2+1];
		value2 = Arial12x12[offset+i*2+2];
		
		buffer[i + posicionL1] = value1;
		buffer[i+128 + posicionL1] = value2;
	}
	posicionL1 += Arial12x12[offset];
	posicionLCD += Arial12x12[offset];
}

 void symbolToLocalBuffer_L2(uint8_t symbol){
	uint8_t i, value1, value2;
	uint16_t offset = 0;
	
	if(flagF2 == 0){
		posicionLCD = 256;
		flagF2 = 1;
	}
	
	offset = 25* (symbol - ' ');
	for (i = 0; i < Arial12x12[offset]; i++){
		if(i + posicionLCD > 383){
				posicionLCD += i;
				return;
		}
		value1 = Arial12x12[offset+i*2+1];
		value2 = Arial12x12[offset+i*2+2];
		
		buffer[i + posicionL2] = value1;
		buffer[i+128 + posicionL2] = value2;
	}
	posicionL2 += Arial12x12[offset];
	posicionLCD += Arial12x12[offset];
}

void symbolToLocalBuffer(uint8_t line, uint8_t symbol){
	if(full == 0){
		uint8_t i, value1, value2;
		uint16_t offset = 0;
		
		offset = 25* (symbol - ' ');
		if(line == 1){ //Cambiar a linea 1
			posicionLCD = posicionL1;
			flagL2 = 0;
		}
		if(line == 2){ //Cambiar a linea 2
			posicionLCD = posicionL2;
			flagL2 = 1;
		}
		if(posicionLCD > 383){ //Si llegamos al final de la linea 2 ya no escribiremos 2
			full = 1;
			return;
		}
		if(symbol == ' ' && posicionLCD == 256){ //Primer caracter de la linea 2 vacio
			return;
		} 
		for (i = 0; i < Arial12x12[offset]; i++){
			if(i + posicionLCD > 127 && flagL2 == 0){
				posicionLCD += i;
				return;
			}
			if(i + posicionLCD > 383){
				posicionLCD += i;
				return;
			}
			value1 = Arial12x12[offset+i*2+1];
			value2 = Arial12x12[offset+i*2+2];
			
			buffer[i + posicionLCD] = value1;
			buffer[i+128 + posicionLCD] = value2;
		}
		posicionLCD += Arial12x12[offset];
		if(line == 1){
			posicionL1 += Arial12x12[offset];
		}
		if(line == 2){
			posicionL2 += Arial12x12[offset];
		}
	}
}

void desplazarAbajo(void){
	for(int i = 0; i < 512; i++){
		buffer2[i] = buffer[i];
	}
	for(int i = 0; i < 512; i++){
		if(i < 128){
			buffer[i] = buffer2[384+i];
		}
		else{
			buffer[i] = buffer2[i-128];
		}
	}
}

void desplazarArriba(void){
	for(int i = 0; i < 512; i++){
		buffer2[i] = buffer[i];
	}
	for(int i = 0; i < 512; i++){
		if(i > 383){
			buffer[i] = buffer2[i-384];
		}
		else{
			buffer[i] = buffer2[i+128];
		}
	}
}

void desplazarDerecha(void){
	for(int i = 0; i < 512; i++){
		buffer2[i] = buffer[i];
	}
	for(int i = 0; i < 128; i++){
		if(i == 0){
			buffer[i] = buffer2[i+127];
			buffer[i+128] = buffer2[i+255];
			buffer[i+256] = buffer2[i+383];
			buffer[i+384] = buffer2[i+511];
		}
		else{
			buffer[i] = buffer2[i-1];
			buffer[i+128] = buffer2[i+127];
			buffer[i+256] = buffer2[i+255];
			buffer[i+384] = buffer2[i+383];
		}
	}
}

void desplazarIzquierda(void){
	for(int i = 0; i < 512; i++){
		buffer2[i] = buffer[i];
	}
	for(int i = 0; i < 128; i++){
		if(i == 127){
			buffer[i] = buffer2[i-127];
			buffer[i+128] = buffer2[i+1];
			buffer[i+256] = buffer2[i+129];
			buffer[i+384] = buffer2[i+257];
		}
		else{
			buffer[i] = buffer2[i+1];
			buffer[i+128] = buffer2[i+129];
			buffer[i+256] = buffer2[i+257];
			buffer[i+384] = buffer2[i+385];
		}
	}
}

int init_LCD(void) {
	lcd = osThreadNew(lcd_Func, NULL, NULL); //Crear el Thread del LCD
	if (lcd == NULL) {
    return(-1);
  }
//	timer = osThreadNew(timer_Func, NULL, NULL); //Crear el Thread del timer de rebotes
//	if (timer == NULL) {
//    return(-1);
//  }
//	rebotes = osThreadNew(rebotes_Func, NULL, NULL); //Crear el Thread de un timer
//	if (rebotes == NULL) {
//    return(-1);
//  }
	LCD_Init();
	LCD_clear();
  return(0);
}

void lcd_Func(void *argument) {
	uint8_t arriba = 0x10,abajo = 0x00;
	uint8_t valor, contador = 0, contador2 = 0;
	uint32_t status;

	while (1) {			
		status = osMessageQueueGet(queue_lcd, &valor, NULL, osWaitForever);   // Esperar Queue
		int numero = valor;
//		status=osThreadFlagsWait(0x1,osFlagsWaitAny,osWaitForever); //Esperar Banderas
		symbolToLocalBuffer(1, numero);
		symbolToLocalBuffer(2, numero);
		LCD_update(); 
		osDelay(500);
	}
}

void Timer_Callback(void *arg){
	osThreadFlagsSet(lcd,0x1); //Flags al LCD
	uint8_t val;
	osMessageQueuePut(queue_lcd, &val, 0U, 0U); //Queue
}

void timer_Func(void *argument) {
	uint32_t status; //Flags de los Threads
	osTimerId_t timsoft1 = osTimerNew(Timer_Callback, osTimerOnce, NULL, NULL); //Crear el timer
	//osTimerStart(timsoft1,2000); //Iniciar el timer	
	while(1){
		status=osThreadFlagsWait(0x1,osFlagsWaitAny,osWaitForever);
		switch (status){
			case 1:
				osTimerStart(timsoft1,2000); //Reiniciar el timer
			break;
		}
	}
}

//Callback del timer
void Trebotes_Callback(void *arg){
//		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET); //LED
//	osThreadFlagsSet(lcd,0x1); //Flags al LCD
//	uint8_t val;
//	osMessageQueuePut(queue_lcd, &val, 0U, 0U); //Queue
	//Lectura de pines
	if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_10) == GPIO_PIN_SET){ //Arriba
	}
	else if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_11) == GPIO_PIN_SET){ //Derecha
	}
	else if(HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_12) == GPIO_PIN_SET){ //Abajo
	}
	else if(HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_14) == GPIO_PIN_SET){ //Izquierda
	}
	else if(HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_15) == GPIO_PIN_SET){ //Centro
	}
}

void rebotes_Func (void *argument) {
	osTimerId_t timer_rebotes = osTimerNew(Trebotes_Callback, osTimerOnce, NULL, NULL); //Crear el timer	
	uint32_t status; //Flags de los Threads
  while (1) {
		status=osThreadFlagsWait(0x1,osFlagsWaitAny,osWaitForever); //Esperar Interrupcion
		switch (status){
			case 1:
				osTimerStart(timer_rebotes,50); //Reiniciar el timer
			break;
		}
  }
}


osThreadId_t lcd_test;  

void lcd_test_Func(void *argument);                   // thread function

int init_LCD_test(void) {
	lcd_test = osThreadNew(lcd_test_Func, NULL, NULL); //Crear el Thread del LCD
	if (lcd_test == NULL) {
    return(-1);
  }
	queue_lcd = osMessageQueueNew(4, sizeof(uint8_t), NULL); //Queue
  return(0);
}

void lcd_test_Func(void *argument) {
	uint8_t letra;
	int iteraciones = 0;
	//letra = 65;
	int vacio = 0;
	char texto[50] = "este es un mensaje muy laaaaaaaargooooooo";
	while (1) {			
		if (primera != 1 && vacio != 1){
			for(int i = 0; i < sizeof(texto); i++){
			letra = texto[i];
	
			if(letra == 0){
				vacio = 1;
				primera = 1;
				break;
			}
			osMessageQueuePut(queue_lcd, &letra, NULL, osWaitForever);
			}
		}
	}
}