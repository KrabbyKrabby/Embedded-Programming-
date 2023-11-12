#include "CLOCK.h"
#include "GPIO.h"
#include "SYS_INIT.h"
#include "USART.h"
#include "stm32f4xx.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char input[100], output[100];
static int input_idx = 0, output_idx = 0;
static int r1 = 12000, g1 = 10000, y1 = 2000, r2 = 12001, g2 = 10001, y2 = 2001, u = 5000;
static int monitor = 10000;
static int sr1 = 0, sg1 = 0, sy1 = 0;
static int sr2 = 0, sg2 = 0, sy2 = 0;
static int east_west_traffic = 0, north_south_traffic = 0;
static int cur_time = 0;

void GPIO_Config(void);
void send_uart4_uart5(void);
void send_uart5_uart4(void);
void config_traffic(int no,char color,int delay);
void send_traffic_1(int g,int y,int r,int ux);
void send_traffic_2(int g,int y,int r,int ux);
void send_traffic_monitor(int m);
void parse_string( void );
void GetString(USART_TypeDef *uart,uint8_t* buff);
void UART4_IRQHandler(void);
void UART5_IRQHandler(void);
void USART2_IRQHandler(void);
void write_up(int no,GPIO_PinState PinState);
void write_down(int no,GPIO_PinState PinState);
void write_left(int no,GPIO_PinState PinState);
void write_right(int no,GPIO_PinState PinState);
void TIM5Config(void);
void TIM2Config(void);
void add_state(int k);
void send_traffic_info(void);
void push(void);
void tim5_delay(uint16_t ms);

void GPIO_Config(void)
{
	RCC -> AHB1ENR |= (1<<0);
	RCC -> AHB1ENR |= (1<<1);
	RCC -> AHB1ENR |= (1<<2);
	RCC -> AHB1ENR |= (1<<3);
	
	GPIO_InitTypeDef InitDef;
	InitDef.Mode = GPIO_MODE_OUTPUT_PP;
	InitDef.Pull = GPIO_NOPULL;
	InitDef.Speed = GPIO_SPEED_FREQ_LOW;
	
	GPIOInit(GPIOA,&InitDef,4);
	GPIOInit(GPIOB,&InitDef,0);
	InitDef.Mode = GPIO_MODE_INPUT;
	GPIOInit(GPIOC,&InitDef,0);
}

void send_uart4_uart5(void)
{
	//UART_SendString(USART2,"4 to 5\n");
	int l = strlen(input);
	for( int i = 0; i < strlen(input); i++ ){
			input_idx = i;
			output_idx = i;
			UART4->CR1 |= (USART_CR1_TXEIE);
			while((UART4->CR1 & USART_CR1_TXEIE));	
			ms_delay(2);
	  }
		output[l] = '\0';
		strcpy(input,"");
}

void send_uart5_uart4(void)
{
	//UART_SendString(USART2,"5 to 4\n");
	
	int l = strlen(input);
	
	for( int i = 0; i < strlen(input); i++ ){
			input_idx = i;
			output_idx = i;
			UART5->CR1 |= (USART_CR1_TXEIE);
			while((UART5->CR1 & USART_CR1_TXEIE));	
			ms_delay(2);
	  }
		output[l] = '\0';
		UART_SendString(USART2,"\n");
		UART_SendString(USART2,output);
		strcpy(input,"");
		strcpy(output,"");
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

	sprintf(input, "traffic light 1 G Y R %d %d %d %d", g,y,r,ux);
	
	send_uart5_uart4();
}

void send_traffic_2(int g,int y,int r,int ux)
{
	strcpy(input,"");
	sprintf(input, "traffic light 2 G Y R %d %d %d %d",  g,y,r,ux);
	
	send_uart5_uart4();
}

void send_traffic_monitor(int m)
{
	
	strcpy(input,"");
	sprintf(input, "traffic monitor %d",m);
	send_uart5_uart4();
}



void parse_string( void )
{
	int l = strlen(output);
	if( output[0] == 'c' ){
		if(output[15] == 'l'){
			char l1, l2, l3;
			int no, x, y, z, tym;
			sscanf(output,"config traffic light %d %c %c %c %d %d %d %d",&no,&l1,&l2,&l3,&x,&y,&z,&tym);
			config_traffic(no,l1,x);
			config_traffic(no,l2,y);
			config_traffic(no,l3,z);
			u = tym;
		}
		else{
			sscanf(output,"config traffic monitor %d",&monitor);
		}
	}
	else if( output[0] == 'r' ){
		if( l == 4 ){
			send_traffic_1(g1,y1,r1,u);
			send_traffic_2(g2,y2,r2,u);
			send_traffic_monitor(monitor);
		}
		else if( output[13] == 'l' ){
			int no;
			sscanf(output,"read traffic light %d",&no);
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
}

void GetString(USART_TypeDef *uart,uint8_t* buff)
{
	uint16_t idx=0;
	uint8_t ch;
	ch = UART_GetChar(USART2);
	while(ch != '.')
	{
		buff[idx]=ch;
		idx++;
		ch = UART_GetChar(uart);
	}
	buff[idx]= '\0';	
}

void UART4_IRQHandler(void)
{
	if( (UART4->SR) & USART_SR_RXNE ){
		output[ output_idx ] = UART4 -> DR;
		UART4->SR &= ~(USART_SR_RXNE);
	}
	
	if(UART4->SR & USART_SR_TXE){
		UART4->DR = input[input_idx];
		
    UART4->SR &= ~USART_SR_TXE;
    UART4->CR1 &= ~(USART_CR1_TXEIE);
	}
	
}

void UART5_IRQHandler(void)
{
	if( (UART5 -> SR)&USART_SR_RXNE ){
		output[ output_idx ] = (uint8_t)UART5 -> DR;
		UART5->SR &= ~(USART_SR_RXNE);	
	}
	
	if(UART5->SR & USART_SR_TXE){
		UART5->DR = input[input_idx];
    
    UART5->SR &= ~USART_SR_TXE;
    UART5->CR1 &= ~(USART_CR1_TXEIE);
	}
}



void USART2_IRQHandler(void)
{
    USART2->CR1 &= ~(USART_CR1_RXNEIE);
		GetString(USART2,input);
    USART2->CR1 |= (USART_CR1_RXNEIE);
}

void write_up(int no,GPIO_PinState PinState)
{
		if( no >= 2 ){
			GPIO_WritePin(GPIOB,GPIO_PIN_0,PinState);
		}
}

void write_down(int no,GPIO_PinState PinState)
{
		if( no >= 2 ){
			GPIO_WritePin(GPIOB,GPIO_PIN_5,PinState);
		}
}

void write_left(int no,GPIO_PinState PinState)
{
		if( no >= 2 ){
			GPIO_WritePin(GPIOA,GPIO_PIN_7,PinState);
		}
}

void write_right(int no,GPIO_PinState PinState)
{
		if( no >= 2 ){
			GPIO_WritePin(GPIOA,GPIO_PIN_10,PinState);
		}
}

void TIM5Config(void)
{
	RCC->APB1ENR |= (1<<3);
	
	TIM5->PSC = 45000 - 1; /* fck = 45 mhz, CK_CNT = fck / (psc[15:0] + 1)*/
	TIM5->ARR = 0xFFFF; /*maximum clock count*/
	
	TIM5->CR1 |= (1<<0);
	
	while(!(TIM5->SR & (1<<0)));
	
}

void TIM2Config(void)
{
	RCC->APB1ENR |= (1<<0);
	
	TIM2->PSC = 45000 - 1; /* fck = 90 mhz, CK_CNT = fck / (psc[15:0] + 1)*/
	TIM2->ARR = 0xFFFF; /*maximum clock count*/
	
	TIM2->CR1 |= (1<<0);
	
	while(!(TIM2->SR & (1<<0)));
	
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
		send_uart5_uart4();
		
		sprintf(input, "%d traffic light 2",arra[i].time_stamp);
		add_state( arra[i].sg2 );
		add_state( arra[i].sy2 );
		add_state( arra[i].sr2 );
		send_uart5_uart4();
		
		if( north_south_traffic == 1 ) sprintf(input, "%d road north south heavy traffic",arra[i].time_stamp);
		else sprintf(input, "%d road north south light traffic",arra[i].time_stamp);
		send_uart5_uart4();
		
		if( east_west_traffic == 1 ) sprintf(input, "%d road east west  heavy traffic",arra[i].time_stamp);
		else sprintf(input, "%d road east west  light traffic",arra[i].time_stamp);
		send_uart5_uart4();
	}
	UART_SendString(USART2,"\n\n");
	
	
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

void tim5_delay(uint16_t ms)
{
	ms = (uint16_t)2 * ms;
	TIM5->CNT = 0;
	
	while(TIM5->CNT < ms){
        
        if(strlen(input) != 0){
					if( input[0] != 't' ){
						send_uart4_uart5();
						parse_string();
						strcpy(output,"");
						strcpy(input,"");
					}	
					
        }
		if(TIM2->CNT > monitor*2){
			cur_time += monitor/1000;
			push();
			send_traffic_info();
			TIM2->CNT = 0;
		}
	}
}







int main( void )
{
	initClock();
	sysInit();
	GPIO_Config();
	TIM5Config();
	TIM2Config();
	UART2_Config();
	UART4_Config();
	UART5_Config();
	
	UART_SendString(USART2,"jello");
	int up, down, left, right;
	while(1){
			
		up = rand()%3;
		down = rand()%3;
		left = rand()%3;
		right = rand()%3;
		
		if( up == 2 || down == 2 ) north_south_traffic = 1; 
		else north_south_traffic = 0;
		
		if( left == 2 || right == 2 ) east_west_traffic = 1;
		else east_west_traffic = 0;
		
		write_left(left,GPIO_PIN_SET); //turning_on_left_vehicles
		write_right(right,GPIO_PIN_SET);//turning_on_right_vehicles
		write_up(up,GPIO_PIN_SET); //turning_on_up_vehicles
		write_down(down,GPIO_PIN_SET);//turning_on_down_vehicles
		
		GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET); //GPIOA_P4_on
		sr1 = 1;
		GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET); //GPIOB_P6_on
		sg2 = 1;
		
		
		//go_B
		tim5_delay(g2);
		//ms_delay(10000);
		//check 0 and 1
		if( left < 2 && right < 2 ){
			tim5_delay(u);//keeping this light on for 5 sec more of low traffic on the other cross road
		}
		
		
		GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET); //GPIOB_P6_off
		sg2 = 0;
		GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET); //GPIOB_P7_on
		sy2 = 1;
		
		tim5_delay(y2);
		
//		
		
		GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET); //GPIOB_P7_off
		sy2 = 0;
		GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);  //GPIOB_P8_on
		sr2 = 1;
		
		GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET); //GPIOA_P4_off
		sr1 = 0;
		
		GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_SET); //GPIOA_P0_on
		sg1 = 1;
		
		
		//go_A
		tim5_delay(g1);
		//ms_delay(10000);
		
		if( up < 2 && down < 2 ){
			tim5_delay(u);//keeping this light on for 5 sec more of low traffic on the other cross road
		}
		
		GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_RESET); //GPIOA_P0_off
		sg1 = 0;
		GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_SET); //GPIOA_P1_on
		sy1 = 1;
		
		tim5_delay(y1);
		GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_RESET); //GPIOA_P1_off
		sy1 = 0;
		GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);  //GPIOB_P8_off
		sr2 = 0;
		
		write_up(up,GPIO_PIN_RESET); //turning_off_up_vehicles
		write_down(down,GPIO_PIN_RESET);//turning_off_down_vehicles
		write_left(left,GPIO_PIN_RESET);//turning_off_left_vehicles
		write_right(right,GPIO_PIN_RESET);//turning_off_right_vehicles
	}
}
	