#include "exti.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "timer.h"
#include "lcd.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F7������
//�ⲿ�ж���������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/11/27
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

//�ⲿ�жϳ�ʼ��
void EXTI_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    //__HAL_RCC_GPIOA_CLK_ENABLE();               //����GPIOAʱ��
    __HAL_RCC_GPIOD_CLK_ENABLE();               //����GPIODʱ��
    //__HAL_RCC_GPIOH_CLK_ENABLE();               //����GPIOHʱ��
    /*
    GPIO_Initure.Pin=GPIO_PIN_0;                //PA0
    GPIO_Initure.Mode=GPIO_MODE_IT_RISING;      //�����ش���
    GPIO_Initure.Pull=GPIO_PULLDOWN;			//����
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
    */
    GPIO_Initure.Pin=GPIO_PIN_3;               //PD3
    GPIO_Initure.Mode=GPIO_MODE_IT_RISING_FALLING;     //˫���ش���
    GPIO_Initure.Pull=GPIO_PULLDOWN;			//����
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);
    /*
    GPIO_Initure.Pin=GPIO_PIN_2;//|GPIO_PIN_3;     //PH2,3  �½��ش���������
    HAL_GPIO_Init(GPIOH,&GPIO_Initure);
    
    //�ж���0
    HAL_NVIC_SetPriority(EXTI0_IRQn,2,0);       //��ռ���ȼ�Ϊ2�������ȼ�Ϊ0
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);             //ʹ���ж���0
    
    //�ж���2
    HAL_NVIC_SetPriority(EXTI2_IRQn,2,1);       //��ռ���ȼ�Ϊ2�������ȼ�Ϊ1
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);             //ʹ���ж���2
    
    //�ж���3
    HAL_NVIC_SetPriority(EXTI3_IRQn,2,2);       //��ռ���ȼ�Ϊ2�������ȼ�Ϊ2
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);             //ʹ���ж���2
    */
    //�ж���4
    HAL_NVIC_SetPriority(EXTI3_IRQn,2,3);   //��ռ���ȼ�Ϊ2�������ȼ�Ϊ1
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);         //ʹ���ж���4
}


//�жϷ�����
void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);   //�����жϴ����ú���
}

void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);   //�����жϴ����ú���
}

void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);   //�����жϴ����ú���
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);  //�����жϴ����ú���
}

//�жϷ����������Ҫ��������
//��HAL�������е��ⲿ�жϷ�����������ô˺���
//GPIO_Pin:�ж����ź�
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    //static u8 led0sta=1,led1sta=1;
    //delay_ms(50);      //����  ����ģ��ʹ�� �������� ��ʱͣ��
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
				tim = __HAL_TIM_GET_COUNTER(&TIM4_Handler);	//��ȡ��ʱ���Ĵ���ֵ
				HAL_TIM_Base_Stop_IT(&TIM4_Handler);
				__HAL_TIM_SET_COUNTER(&TIM4_Handler,0x00);
				
				//LCD_ShowxNum(60+32,330,tim,5,16,0);							//��ʾ���� 
            }
            break;
    }
}
