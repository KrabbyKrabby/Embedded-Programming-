#include "stm32f4xx.h"
#include "CLOCK.h"
#include "SYS_INIT.h"
#include "GPIO.h"
#include "stdlib.h"

void GPIO_Config(void)
{
	
	//initialize bus
	RCC -> AHB1ENR |= (1<<0);
	RCC -> AHB1ENR |= (1<<1);
	RCC -> AHB1ENR |= (1<<2);
	
	GPIO_InitTypeDef InitDef;
	InitDef.Mode = GPIO_MODE_OUTPUT_PP;
	InitDef.Pull = GPIO_NOPULL;
	InitDef.Speed = GPIO_SPEED_FREQ_LOW;
	
	GPIO_Init(GPIOA,&InitDef);
	GPIO_Init(GPIOB,&InitDef);
	InitDef.Mode = GPIO_MODE_INPUT;
	GPIO_Init(GPIOC,&InitDef);
}

void traffic_flow_delay_ms_x( int tm )
{
	int it = tm/1000;
	for( int i = 1; i <= it; i++ )
	{
		GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_SET);
		GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_SET);
		
		ms_delay(333);
		
		GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_RESET);
		GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_RESET);
		
		GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_SET);
		GPIO_WritePin(GPIOA,GPIO_PIN_9,GPIO_PIN_SET);
		
		ms_delay(333);
		
		GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_RESET);
		GPIO_WritePin(GPIOA,GPIO_PIN_9,GPIO_PIN_RESET);
		
		GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_SET);
		GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_SET);
		
		ms_delay(334);
		
		GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_RESET);
		GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET);
	}
}

void traffic_flow_delay_ms_y( int tm )
{
	int it = tm/1000;
	for( int i = 1; i <= it; i++ )
	{
		GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);
		GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET);
		
		ms_delay(333);
		
		GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);
		GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_RESET);
		
		GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);
		GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_SET);
		
		ms_delay(333);
		
		GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
		GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);
		
		GPIO_WritePin(GPIOB,GPIO_PIN_2,GPIO_PIN_SET);
		GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_SET);
		
		ms_delay(334);
		
		GPIO_WritePin(GPIOB,GPIO_PIN_2,GPIO_PIN_RESET);
		GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_RESET);
	}
}

void write_up(int no,GPIO_PinState PinState)
{
		if( no >= 0 ){
			GPIO_WritePin(GPIOB,GPIO_PIN_2,PinState);
		}
		if( no >= 1 ){
			GPIO_WritePin(GPIOB,GPIO_PIN_1,PinState);
		}
		if( no >= 2 ){
			GPIO_WritePin(GPIOB,GPIO_PIN_0,PinState);
		}
}

void write_down(int no,GPIO_PinState PinState)
{
		if( no >= 0 ){
			GPIO_WritePin(GPIOB,GPIO_PIN_3,PinState);
		}
		if( no >= 1 ){
			GPIO_WritePin(GPIOB,GPIO_PIN_4,PinState);
		}
		if( no >= 2 ){
			GPIO_WritePin(GPIOB,GPIO_PIN_5,PinState);
		}
}

void write_left(int no,GPIO_PinState PinState)
{
		if( no >= 0 ){
			GPIO_WritePin(GPIOA,GPIO_PIN_5,PinState);
		}
		if( no >= 1 ){
			GPIO_WritePin(GPIOA,GPIO_PIN_6,PinState);
		}
		if( no >= 2 ){
			GPIO_WritePin(GPIOA,GPIO_PIN_7,PinState);
		}
}

void write_right(int no,GPIO_PinState PinState)
{
		if( no >= 0 ){
			GPIO_WritePin(GPIOA,GPIO_PIN_8,PinState);
		}
		if( no >= 1 ){
			GPIO_WritePin(GPIOA,GPIO_PIN_9,PinState);
		}
		if( no >= 2 ){
			GPIO_WritePin(GPIOA,GPIO_PIN_10,PinState);
		}
}

int main(void)
{
	initClock();
	sysInit();
	GPIO_Config();
	
	srand(83);
	
	int up, down, left, right;
	
	while(1)
	{
		
		up = rand()%3;
		down = rand()%3;
		left = rand()%3;
		right = rand()%3;
		
		write_left(left,GPIO_PIN_SET); //turning_on_left_vehicles
		write_right(right,GPIO_PIN_SET);//turning_on_right_vehicles
		
		GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET); //GPIOA_P4_on
		if( ((GPIOC -> IDR)&(1<<5)) != 0 ) GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET); //GPIOB_P6_on
		
		
		//go_B
		traffic_flow_delay_ms_y(10000);
		//ms_delay(10000);
		//check 0 and 1
		if( (((GPIOC -> IDR)&(1<<0)) == 0) && (((GPIOC -> IDR)&(1<<1)) == 0) ){
			traffic_flow_delay_ms_y(5000);//keeping this light on for 5 sec more of low traffic on the other cross road
		}
		
		
		GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET); //GPIOB_P6_off
		GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET); //GPIOB_P7_on
		
		traffic_flow_delay_ms_y(2000);
		
		write_left(left,GPIO_PIN_RESET);//turning_off_left_vehicles
		write_right(right,GPIO_PIN_RESET);//turning_off_right_vehicles
		
		GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET); //GPIOB_P7_off
		GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);  //GPIOB_P8_on
		
		GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET); //GPIOA_P4_off
		
		if( ((GPIOC -> IDR)&(1<<6)) != 0 ) GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET); //GPIOA_P0_on
		
		write_up(up,GPIO_PIN_SET); //turning_on_up_vehicles
		write_down(down,GPIO_PIN_SET);//turning_on_down_vehicles
		
		//go_A
		traffic_flow_delay_ms_x(10000);
		//ms_delay(10000);
		
		if( ((GPIOC -> IDR)&(1<<2)) == 0 && ((GPIOC -> IDR)&(1<<3)) == 0 ){
			traffic_flow_delay_ms_x(5000);//keeping this light on for 5 sec more of low traffic on the other cross road
		}
		
		GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET); //GPIOA_P0_off
		GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_SET); //GPIOA_P1_on
		
		traffic_flow_delay_ms_x(2000);
		GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_RESET); //GPIOA_P1_off
		GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);  //GPIOB_P8_off
		
		write_up(up,GPIO_PIN_RESET); //turning_off_up_vehicles
		write_down(down,GPIO_PIN_RESET);//turning_off_down_vehicles
		
	}
}
