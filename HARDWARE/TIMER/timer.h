#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F7������
//��ʱ����������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/11/27
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 
extern TIM_HandleTypeDef TIM3_Handler;      //��ʱ��3PWM��� 
extern TIM_HandleTypeDef TIM4_Handler;      //��ʱ��4PWM��� 
extern unsigned long int time_counter;

void TIM3_Init(u16 arr,u16 psc);    //��ʱ����ʼ��
void TIM4_Init(u16 arr,u16 psc);    //��ʱ����ʼ��


#endif

