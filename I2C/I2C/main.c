#include "GPIO.h"
#include "USART.h"
#include "SYS_INIT.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int input_idx = 0;

char input[100];

void getString(void){
    uint8_t ch,idx = 0;
    ch = UART_GetChar(USART2);
    while(ch != '.'){
        input[idx++] = ch;
        ch = UART_GetChar(USART2);
        if(ch == '.')break;
    }      
    input[idx] = '\0';
}
void USART2_IRQHandler(void){
    USART2->CR1 &= ~(USART_CR1_RXNEIE);
    getString();
    USART2->CR1 |= (USART_CR1_RXNEIE);
}

void I2C_ReceiveSlave(void){
    int idx = 0,size = 0;
    int ch = 0;

    while(!(I2C1->SR1 & I2C_SR1_ADDR)); /*wait for address to set*/
    idx = I2C1->SR1 | I2C1->SR2; /*clear address flag*/
    
    idx = 0;
    while(ch != '!'){
        while(!(I2C1->SR1 & I2C_SR1_RXNE)); /*wait for rxne to set*/
        ch = (uint8_t)I2C1->DR;/*read data from DR register*/
        if(ch == '!')break;
        input[idx++] = ch;
        size++;
    }
    input[idx] = '\0';
    
    while(!(I2C1->SR1 & I2C_SR1_STOPF)); /*wait for stopf to set*/
    /*clear stop flag*/
    idx = I2C1->SR1; 
    I2C1->CR1 |= I2C_CR1_PE;
    
    I2C1->CR1 &= ~I2C_CR1_ACK; /*disable ack*/
}

void I2C1_EV_IRQHandler(void){ 
    
    I2C1->CR2 &= ~I2C_CR2_ITEVTEN;

    if(I2C1->SR1 & I2C_SR1_ADDR) I2C_ReceiveSlave();     


    if(strlen(input) != 0){
				UART_SendString(USART2,input);
				UART_SendString(USART2,"\n");
        //parseCommand(input);
				strcpy(input,"");
    }

    I2C1->CR2 |= I2C_CR2_ITEVTEN;
}


void TIM2Config(void)
{
	RCC->APB1ENR |= (1<<0);
	
	TIM2->PSC = 45000 - 1; /* fck = 90 mhz, CK_CNT = fck / (psc[15:0] + 1)*/
	TIM2->ARR = 0xFFFF; /*maximum clock count*/
	
	TIM2->CR1 |= (1<<0);
	
	while(!(TIM2->SR & (1<<0)));
	
}

int I2C_Start(void)
{
	I2C1 -> CR1 |= (1<<8);
	TIM2 -> CNT = 0;
	while( !(I2C1 -> SR1 & (1<<0)) ){
		if( TIM2 -> CNT > 200 ){
			return 1;
		}
	}
	return 0;
}

int I2C_Address(int address)
{
	I2C1->DR = (uint8_t)(address << 1);
	TIM2 -> CNT = 0;
	
	while(!(I2C1->SR1 & I2C_SR1_ADDR)){
		char str[100];
		UART_SendString(USART2,str);
        if(TIM2->CNT > 200){
            return 1;
        }   
    }
	
	int temp = (I2C1 -> SR1) | (I2C1 -> SR2);
	
	return 0;
}

int I2C_Write(int data)
{
	TIM2 -> CNT = 0;
	while( !(I2C1 -> SR1 & (1<<7)) ){
		if( TIM2 -> CNT > 200 ){
			return 1;
		}
	}
	
	I2C1 -> DR = data;
	
	TIM2 -> CNT = 0;
	while( !( I2C1 -> SR1 & (1<<2 )) ){
		if( TIM2 -> CNT > 200 ){
			return 1;
		}
	}
	return 0;
}

int I2C_Stop()
{
	I2C1 -> CR1 |= (1<<9);
	return 0;
}

void I2C1_SetAddress(int address){
    I2C1->OAR1 |= (uint32_t)(address << 1U);
    I2C1->OAR1 &= ~I2C_OAR1_ADDMODE;
}

void I2C_Send_Master_Slave()
{
		int f = 1;
		while(f == 1){
			UART_SendString(USART2,"1\n");
			f = 0;
			f |= I2C_Start();
			UART_SendString(USART2,"2\n");
			f |= I2C_Address(0);
			UART_SendString(USART2,"3\n");
			for(int idx = 0;idx < strlen(input);idx++) f |= I2C_Write(input[idx]);
			
			UART_SendString(USART2,"4\n");
			f |= I2C_Stop();
			UART_SendString(USART2,"5\n");
		}
}

void I2C_config(int mode)
{
	RCC -> APB1ENR |= (1<<21); //I2C1_EN
	RCC -> AHB1ENR |= (1<<1); //GPIOB_EN
	
	//Configure PB8 and PB9
	GPIOB -> MODER |= (2<<16) | (2<<18);
	GPIOB -> OTYPER |= (1<<8) | (1<<9);
	GPIOB -> OSPEEDR |= (3<<16) | (3<<18);
	GPIOB -> PUPDR |= (1<<16) | (1<<18);
	GPIOB -> AFR[1] |= (4<<0) | (4<<4);
	
	//Reset I2C
	I2C1 -> CR1 |= (1<<15); 
	I2C1 -> CR1 &= ~(1<<15);
	
	//Configure clock control register
	I2C1->CR2 |= 45 << 0;
  I2C1->CCR |= 225 << 0;
  I2C1->TRISE = 46;
	
	//Peripheral_Enable
	I2C1 ->CR1 |= 1<<0;
	
	if(mode == 0){
        /*Set Address and Enable Interrupts for Slave Receieve*/
        I2C1_SetAddress(0);
        I2C1->CR2 |= I2C_CR2_ITEVTEN; /*Enable Event Interrupt*/      
        NVIC_EnableIRQ(I2C1_EV_IRQn);  
    }
	
}

void init(void)
{
	initClock();
	sysInit();
	UART2_Config();
}

int main(void)
{
	
	init();
	int master = 0;
	I2C_config(master);
	if( master == 1 ){
		UART_SendString(USART2,"Master\n");
		while(1){
			if( strlen(input) != 0 ){
				I2C_Send_Master_Slave();
				strcpy(input,"");
			}
		}
	}
	else{
		UART_SendString(USART2,"Slave\n");
		while(1){
			I2C1->CR1 |= I2C_CR1_ACK; 
		}
	}
}