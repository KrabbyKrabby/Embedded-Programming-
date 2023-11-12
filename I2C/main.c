#include "GPIO.h"
#include "USART.h"
#include "SYS_INIT.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int input_idx = 0;
char input[100];

static int r1 = 12000, g1 = 10000, y1 = 2000, r2 = 12001, g2 = 10001, y2 = 2001, u = 5000;
static int monitor = 10000;
static int sr1 = 0, sg1 = 0, sy1 = 0;
static int sr2 = 0, sg2 = 0, sy2 = 0;
static int east_west_traffic = 0, north_south_traffic = 0;
static int cur_time = 0;



void send_traffic_info(void);
void push(void);
void parse_string( void );

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

    if(I2C1->SR1 & I2C_SR1_ADDR){
			I2C_ReceiveSlave(); 
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

void TIM5Config(void)
{
	RCC->APB1ENR |= (1<<3);
	
	TIM5->PSC = 45000 - 1; /* fck = 45 mhz, CK_CNT = fck / (psc[15:0] + 1)*/
	TIM5->ARR = 0xFFFF; /*maximum clock count*/
	
	TIM5->CR1 |= (1<<0);
	
	while(!(TIM5->SR & (1<<0)));
	
}

void delay(int ms)
{
	ms *= 2;
	TIM5 -> CNT = 0;
	while(TIM5 -> CNT <= ms){
		I2C1->CR1 |= I2C_CR1_ACK; 
		
		if(strlen(input) != 0){
			parse_string();
			strcpy(input,"");
		}
		
		if(TIM2->CNT > monitor*2){
			cur_time += monitor/1000;
			push();
			send_traffic_info();
			TIM2->CNT = 0;
		}
		
	}
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
			f = 0;
			//UART_SendString(USART2,"1\n");
			f |= I2C_Start();
		//	UART_SendString(USART2,"2\n");
			f |= I2C_Address(0);
		//	UART_SendString(USART2,"3\n");
			for(int idx = 0;idx < strlen(input);idx++) f |= I2C_Write(input[idx]);
			//UART_SendString(USART2,"4\n");
			
			f |= I2C_Stop();
		//	UART_SendString(USART2,"5\n");
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



void GPIO_Config(void)
{
	RCC -> AHB1ENR |= (1<<2);
	GPIO_InitTypeDef InitDef;
	InitDef.Mode = GPIO_MODE_OUTPUT_PP;
	InitDef.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIOInit(GPIOC,&InitDef);
}

void config_traffic(int no,char color,int delay)
{
	if( no == 1 ){
		if( color == 'R' ){
			r1 = delay;
		}
		else if( color == 'G' ){
			g1 = delay;
		}
		else if( color == 'Y' ){
			y1 = delay;
		}			
	}
	else{
		if( color == 'R' ){
			r2 = delay;
		}
		else if( color == 'G' ){
			g2 = delay;
		}
		else if( color == 'Y' ){
			y2 = delay;
		}			
	}
	
}

void send_traffic_1(int g,int y,int r,int ux)
{
	strcpy(input,"");

	sprintf(input, "traffic light 1 G Y R %d %d %d %d\n", g,y,r,ux);
	
	UART_SendString(USART2,input);
	strcpy(input,"");
}

void send_traffic_2(int g,int y,int r,int ux)
{
	strcpy(input,"");
	sprintf(input, "traffic light 2 G Y R %d %d %d %d\n",  g,y,r,ux);
	
	UART_SendString(USART2,input);
	strcpy(input,"");
}

void send_traffic_monitor(int m)
{
	strcpy(input,"");
	sprintf(input, "traffic monitor %d\n",m);
	UART_SendString(USART2,input);
	strcpy(input,"");
}



void parse_string( void )
{
	int l = strlen(input);
	if( input[0] == 'c' ){
		if(input[15] == 'l'){
			char l1, l2, l3;
			int no, x, y, z, tym;
			sscanf(input,"config traffic light %d %c %c %c %d %d %d %d",&no,&l1,&l2,&l3,&x,&y,&z,&tym);
			config_traffic(no,l1,x);
			config_traffic(no,l2,y);
			config_traffic(no,l3,z);
			u = tym;
		}
		else{
			sscanf(input,"config traffic monitor %d",&monitor);
		}
	}
	else if( input[0] == 'r' ){
		if( l == 4 ){
			send_traffic_1(g1,y1,r1,u);
			send_traffic_2(g2,y2,r2,u);
			send_traffic_monitor(monitor);
		}
		else if( input[13] == 'l' ){
			int no;
			sscanf(input,"read traffic light %d",&no);
			if( no == 1 ){
				send_traffic_1(g1,y1,r1,u);
			}
			else{
				send_traffic_2(g2,y2,r2,u);
			}
		}
		else{
			send_traffic_monitor(monitor);
		}
	}
	strcpy(input,"");
}

struct state
{
	int time_stamp;
	int sr1, sg1, sy1;
	int sr2, sg2, sy2;
	int east_west_traffic, north_south_traffic;
};

int sz = 0;
static struct state arra[5];

void add_state(int k)
{
	if( k == 1 ) strcat(input," ON");
	else strcat(input," OFF");
}

void send_traffic_info(void)
{
	for( int i = 0; i < sz; i++ ){
		sprintf(input, "%d traffic light 1",arra[i].time_stamp);
		add_state( arra[i].sg1 );
		add_state( arra[i].sy1 );
		add_state( arra[i].sr1 );
		UART_SendString(USART2,input);
		UART_SendString(USART2,"\n");
		
		sprintf(input, "%d traffic light 2",arra[i].time_stamp);
		add_state( arra[i].sg2 );
		add_state( arra[i].sy2 );
		add_state( arra[i].sr2 );
		UART_SendString(USART2,input);
		UART_SendString(USART2,"\n");
		
		if( north_south_traffic == 1 ) sprintf(input, "%d road north south heavy traffic",arra[i].time_stamp);
		else sprintf(input, "%d road north south light traffic",arra[i].time_stamp);
		UART_SendString(USART2,input);
		UART_SendString(USART2,"\n");
		
		if( east_west_traffic == 1 ) sprintf(input, "%d road east west  heavy traffic",arra[i].time_stamp);
		else sprintf(input, "%d road east west  light traffic",arra[i].time_stamp);
		UART_SendString(USART2,input);
		UART_SendString(USART2,"\n");
	}
	UART_SendString(USART2,"\n\n");
	strcpy(input,"");
	
}

void push(void)
{
	struct state temp;
	temp.time_stamp = cur_time;
	
	temp.sr1 = sr1;
	temp.sg1 = sg1;
	temp.sy1 = sy1;
		
	temp.sr2 = sr2;
	temp.sg2 = sg2;
	temp.sy2 = sy2;
		
	temp.east_west_traffic = east_west_traffic;
	temp.north_south_traffic = north_south_traffic;
	if( sz <= 2  ) arra[sz++] = temp;
	else{
		for( int i = 0; i < 2; i++ ) arra[i] = arra[i+1];
		sz--;
		arra[sz++] = temp;
	}
}

void init(void)
{
	initClock();
	sysInit();
	UART2_Config();
	GPIO_Config();
	TIM2Config();
	TIM5Config();
}

void Traffic_loop(void)
{
	int one = 0, two = 0;
	while(1)
	{
		one = rand()%3;
		two = rand()%3;
		
		if( one == 2 ){ 
			east_west_traffic = 1;
			GPIO_WritePin(GPIOC,GPIO_PIN_5,GPIO_PIN_SET);
		}
		else east_west_traffic = 0;
		
		if( two == 2 ){ 
			north_south_traffic = 1; 
			GPIO_WritePin(GPIOC,GPIO_PIN_6,GPIO_PIN_SET);
		}
		else north_south_traffic = 0;
		
		GPIO_WritePin(GPIOC,GPIO_PIN_1,GPIO_PIN_SET);
		sr1 = 1;
		GPIO_WritePin(GPIOC,GPIO_PIN_4,GPIO_PIN_SET);
		sg2 = 1;
		
		delay(g2);
		if( one < 2 ){
			delay(u);//keeping this light on for 5 sec more of low traffic on the other cross road
		}
		
		GPIO_WritePin(GPIOC,GPIO_PIN_4,GPIO_PIN_RESET);
		sg2 = 0;
	//	GPIO_WritePin(GPIOC,GPIO_PIN_8,GPIO_PIN_SET);
		sy2 = 1;
		
		delay(y2);
		
		//GPIO_WritePin(GPIOC,GPIO_PIN_8,GPIO_PIN_RESET);
		sy2 = 0;
		
		GPIO_WritePin(GPIOC,GPIO_PIN_3,GPIO_PIN_SET);
		sr2 = 1;
		
		GPIO_WritePin(GPIOC,GPIO_PIN_1,GPIO_PIN_RESET);
		sr1 = 0;
		
		GPIO_WritePin(GPIOC,GPIO_PIN_2,GPIO_PIN_SET); 
		sg1 = 1;
		
		delay(g1);
		
		if( two < 2 ){
			delay(u);//keeping this light on for 5 sec more of low traffic on the other cross road
		}
		
		GPIO_WritePin(GPIOC,GPIO_PIN_2,GPIO_PIN_RESET); 
		sg1 = 0;
		//GPIO_WritePin(GPIOC,GPIO_PIN_1,GPIO_PIN_SET); 
		sy1 = 1;
		
		delay(y1);
		
	//	GPIO_WritePin(GPIOC,GPIO_PIN_1,GPIO_PIN_RESET); 
		sy1 = 0;
		GPIO_WritePin(GPIOC,GPIO_PIN_3,GPIO_PIN_RESET);  
		sr2 = 0;
		
		GPIO_WritePin(GPIOC,GPIO_PIN_5,GPIO_PIN_RESET);
		GPIO_WritePin(GPIOC,GPIO_PIN_6,GPIO_PIN_RESET);
		
	}
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
			Traffic_loop();
		}
	}
}