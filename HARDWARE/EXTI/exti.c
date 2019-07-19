#include "exti.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "timer.h"
#include "lcd.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F7开发板
//外部中断驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/11/27
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

//外部中断初始化
void EXTI_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    //__HAL_RCC_GPIOA_CLK_ENABLE();               //开启GPIOA时钟
    __HAL_RCC_GPIOD_CLK_ENABLE();               //开启GPIOD时钟
    //__HAL_RCC_GPIOH_CLK_ENABLE();               //开启GPIOH时钟
    /*
    GPIO_Initure.Pin=GPIO_PIN_0;                //PA0
    GPIO_Initure.Mode=GPIO_MODE_IT_RISING;      //上升沿触发
    GPIO_Initure.Pull=GPIO_PULLDOWN;			//下拉
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
    */
    GPIO_Initure.Pin=GPIO_PIN_3;               //PD3
    GPIO_Initure.Mode=GPIO_MODE_IT_RISING_FALLING;     //双边沿触发
    GPIO_Initure.Pull=GPIO_PULLDOWN;			//下拉
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);
    /*
    GPIO_Initure.Pin=GPIO_PIN_2;//|GPIO_PIN_3;     //PH2,3  下降沿触发，上拉
    HAL_GPIO_Init(GPIOH,&GPIO_Initure);
    
    //中断线0
    HAL_NVIC_SetPriority(EXTI0_IRQn,2,0);       //抢占优先级为2，子优先级为0
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);             //使能中断线0
    
    //中断线2
    HAL_NVIC_SetPriority(EXTI2_IRQn,2,1);       //抢占优先级为2，子优先级为1
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);             //使能中断线2
    
    //中断线3
    HAL_NVIC_SetPriority(EXTI3_IRQn,2,2);       //抢占优先级为2，子优先级为2
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);             //使能中断线2
    */
    //中断线4
    HAL_NVIC_SetPriority(EXTI3_IRQn,2,3);   //抢占优先级为2，子优先级为1
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);         //使能中断线4
}


//中断服务函数
void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);   //调用中断处理公用函数
}

void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);   //调用中断处理公用函数
}

void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);   //调用中断处理公用函数
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);  //调用中断处理公用函数
}

//中断服务程序中需要做的事情
//在HAL库中所有的外部中断服务函数都会调用此函数
//GPIO_Pin:中断引脚号
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    //static u8 led0sta=1,led1sta=1;
    //delay_ms(50);      //消抖  搭配模块使用 无需消抖 暂时停用
    switch(GPIO_Pin)
    {
        case GPIO_PIN_3:
            if(PD3_STA ==1)	
            {
				//LED1(0);
				HAL_TIM_Base_Start_IT(&TIM4_Handler);
            }
			 if(PD3_STA ==0)	
            {
				//LED1(1);
				tim = __HAL_TIM_GET_COUNTER(&TIM4_Handler);	//读取定时器寄存器值
				HAL_TIM_Base_Stop_IT(&TIM4_Handler);
				__HAL_TIM_SET_COUNTER(&TIM4_Handler,0x00);
				
				//LCD_ShowxNum(60+32,330,tim,5,16,0);							//显示距离 
            }
            break;
    }
}
