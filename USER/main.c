#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "sdram.h"
#include "pcf8574.h"
#include "ap3216c.h"
#include "exti.h"
#include "timer.h"
#include "24cxx.h"
#include "w25qxx.h"
#include "qspi.h"
#include "touch.h"

#define SPISave 1			//ʵ�ֵ��籣��ķ�ʽ  Ϊ1ʱ��ʹ��SPI��дW25Q256  Ϊ0ʱ ʹ��I2C��д24C02

// ������Trig��PA5  Echo��PD3  

int set_flag = 0;			//�����ж��õ�ȫ�ֱ��� ��ʱͣ��
u16 tim;					//���ڱ��泬����ģ�鶨ʱ����ֵ

unsigned long int time_counter = 0;		//��ʱ���жϼ�ʱ�õ�ȫ�ֱ��� ��������
int setting = 0;						//�˵����ܱ�־λ

	u8 t=0;
	u8 i=0;	  	    
 	u16 lastpos[10][2];		//���һ�ε����� 
	u8 maxp=5;


//�����Ļ�������Ͻ���ʾ"RST"
void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);//����   
 	POINT_COLOR=BLUE;//��������Ϊ��ɫ 
	LCD_ShowString(lcddev.width-24,0,200,16,16,"RST");//��ʾ��������
  	POINT_COLOR=RED;//���û�����ɫ 
}
////////////////////////////////////////////////////////////////////////////////
//���ݴ�����ר�в���
//��ˮƽ��
//x0,y0:����
//len:�߳���
//color:��ɫ
void gui_draw_hline(u16 x0,u16 y0,u16 len,u16 color)
{
	if(len==0)return;
	LCD_Fill(x0,y0,x0+len-1,y0,color);	
}
//��ʵ��Բ
//x0,y0:����
//r:�뾶
//color:��ɫ
void gui_fill_circle(u16 x0,u16 y0,u16 r,u16 color)
{											  
	u32 i;
	u32 imax = ((u32)r*707)/1000+1;
	u32 sqmax = (u32)r*(u32)r+(u32)r/2;
	u32 x=r;
	gui_draw_hline(x0-r,y0,2*r,color);
	for (i=1;i<=imax;i++) 
	{
		if ((i*i+x*x)>sqmax)// draw lines from outside  
		{
 			if (x>imax) 
			{
				gui_draw_hline (x0-i+1,y0+x,2*(i-1),color);
				gui_draw_hline (x0-i+1,y0-x,2*(i-1),color);
			}
			x--;
		}
		// draw lines from inside (center)  
		gui_draw_hline(x0-x,y0+i,2*x,color);
		gui_draw_hline(x0-x,y0-i,2*x,color);
	}
}  
//������֮��ľ���ֵ 
//x1,x2����ȡ��ֵ��������
//����ֵ��|x1-x2|
u16 my_abs(u16 x1,u16 x2)
{			 
	if(x1>x2)return x1-x2;
	else return x2-x1;
}  
//��һ������
//(x1,y1),(x2,y2):��������ʼ����
//size�������Ĵ�ϸ�̶�
//color����������ɫ
void lcd_draw_bline(u16 x1, u16 y1, u16 x2, u16 y2,u8 size,u16 color)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	if(x1<size|| x2<size||y1<size|| y2<size)return; 
	delta_x=x2-x1; //������������ 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //���õ������� 
	else if(delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//ˮƽ�� 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //ѡȡ�������������� 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//������� 
	{  
		gui_fill_circle(uRow,uCol,size,color);//���� 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}   
////////////////////////////////////////////////////////////////////////////////
//10�����ص����ɫ(���ݴ�������)												 
const u16 POINT_COLOR_TBL[10]={RED,GREEN,BLUE,BROWN,GRED,BRED,GBLUE,LIGHTBLUE,BRRED,GRAY};  
//���败�������Ժ���
void rtp_test(void)
{
	u8 key;
	u8 i=0;	  
	while(1)
	{
	 	key=KEY_Scan(0);
		tp_dev.scan(0); 		 
		if(tp_dev.sta&TP_PRES_DOWN)			//������������
		{	
		 	if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.x[0]>(lcddev.width-24)&&tp_dev.y[0]<16)Load_Drow_Dialog();//���
				else TP_Draw_Big_Point(tp_dev.x[0],tp_dev.y[0],RED);		//��ͼ	  			   
			}
		}else delay_ms(10);	//û�а������µ�ʱ�� 	    
		if(key==KEY0_PRES)	//KEY0����,��ִ��У׼����
		{
			LCD_Clear(WHITE);	//����
		    TP_Adjust();  		//��ĻУ׼ 
			TP_Save_Adjdata();	 
			Load_Drow_Dialog();
		}
		i++;
		if(i%20==0)LED0_Toggle;
	}
}
//���ݴ��������Ժ���
void ctp_test(void)
{
	//u8 t=0;
	//u8 i=0;	  	    
 	//u16 lastpos[10][2];		//���һ�ε����� 
	//u8 maxp=5;
	if(lcddev.id==0X1018)
		maxp=10;
	//while(1)
	//{
		tp_dev.scan(0);
		for(t=0;t<maxp;t++)
		{
			if((tp_dev.sta)&(1<<t))
			{
				if(tp_dev.x[t]<lcddev.width&&tp_dev.y[t]<lcddev.height)
				{
					if(lastpos[t][0]==0XFFFF)
					{
						lastpos[t][0] = tp_dev.x[t];
						lastpos[t][1] = tp_dev.y[t];
					}
					//lcd_draw_bline(lastpos[t][0],lastpos[t][1],tp_dev.x[t],tp_dev.y[t],2,POINT_COLOR_TBL[t]);//����
					lastpos[t][0]=tp_dev.x[t];
					lastpos[t][1]=tp_dev.y[t];
					if(15<tp_dev.x[t]&&tp_dev.x[t]<112&&410<tp_dev.y[t]&&tp_dev.y[t]<470)
					{
						//Load_Drow_Dialog();//���
						//LCD_DrawRectangle(15,410,112,470);
						if(setting == 0)
						{
							setting=1;
						}
						else if(setting == 1)
						{
							setting=2;
						}
						else if(setting == 2)
						{
							setting=0;
						}
						LED1(0);					
					}
					if(135<tp_dev.x[t]&&tp_dev.x[t]<232&&410<tp_dev.y[t]&&tp_dev.y[t]<470)
					{
						//Load_Drow_Dialog();//���
						//LCD_DrawRectangle(135,410,232,470);
						LED1(1);					
					}
				
				}
				
				
			}else lastpos[t][0]=0XFFFF;
		} 
		delay_ms(5);i++;
		//if(i%20==0)LED0_Toggle;
	//}	
}


int main(void)
{
	
	u8 mmm = 10;
	u8 time_value = 1;
	u8 dis_set = 50;
	float dis_value;
	u8 beepuse = 0;
	u16 ir,als,ps;   
    u8 key;
	int key2;
	u16 i=0; 
	u8 beepsta=1;    
    Cache_Enable();                 //��L1-Cache
    HAL_Init();				        //��ʼ��HAL��
    Stm32_Clock_Init(432,25,2,9);   //����ʱ��,216Mhz 
    delay_init(216);                //��ʱ��ʼ��
	uart_init(115200);		        //���ڳ�ʼ��
    LED_Init();                     //��ʼ��LED
	
    KEY_Init();                     //��ʼ������
    SDRAM_Init();                   //��ʼ��SDRAM
    LCD_Init();                     //LCD��ʼ��
	W25QXX_Init();		            //��ʼ��W25QXX
	EXTI_Init();					//�жϳ�ʼ�� ��ʱͣ��
	TIM3_Init(10000-1,10800-1);      //ϵͳע�ͣ���ʱ��3��ʼ������ʱ��ʱ��Ϊ108M����Ƶϵ��Ϊ10800-1��
									 //ϵͳע�ͣ����Զ�ʱ��3��Ƶ��Ϊ108M/10800=10K���Զ���װ��Ϊ5000-1����ô��ʱ�����ھ���500ms
									 //50000-1 5��һ�ж�  10000-1 1��һ�ж�
									 //��ʱ��3����ϵͳ��ʱ
	TIM4_Init(50000-1,10800-1); 	 //��ʱ��4���ڳ�����ģ������ֵ
	tp_dev.init();				    //��������ʼ�� 
	//HAL_TIM_Base_Start_IT(&TIM4_Handler);
	
	//if(tp_dev.touchtype!=0XFF)LCD_ShowString(30,130,200,16,16,"Press KEY0 to Adjust");//����������ʾ
	//delay_ms(1500);
 	//Load_Drow_Dialog();	 	
	
	//if(tp_dev.touchtype&0X80)ctp_test();//����������
	//else rtp_test(); 					//����������  									  	       

	
	POINT_COLOR=RED;
	LCD_ShowString(20,30,200,32,16,"Vision protector");	
	LCD_ShowString(20,50,200,16,16,"MuZhi@xsyu 2019/6/18"); 
	
	LCD_ShowString(20,90,200,16,16,"KEY2/KEY0: Add/Less");			//��ʾ��ʾ��Ϣ	
	LCD_ShowString(20,110,200,16,16,"KEY_UP: Switch");		//��ʾ��ʾ��Ϣ		
	LCD_ShowString(20,130,200,16,16,"KEY1: Enter");		//��ʾ��ʾ��Ϣ		
	
	while(PCF8574_Init())		//��ⲻ��PCF8574
	{
		LCD_ShowString(20,170,200,16,16,"PCF8574 Check Failed!");
		delay_ms(500);
		LCD_ShowString(20,170,200,16,16,"Please Check!      ");
		delay_ms(500);
		LED0_Toggle;//DS0��˸
	}
	LCD_ShowString(20,170,200,16,16,"PCF8574 OK");    
	//POINT_COLOR=BLUE;//��������Ϊ��ɫ
	
	while(AP3216C_Init())		//��ⲻ��AP3216C
	{
		LCD_ShowString(20,170,200,16,16,"AP3216C Check Failed!");
		delay_ms(500);
		LCD_ShowString(20,170,200,16,16,"Please Check!        ");
		delay_ms(500);
		LED0_Toggle;				//DS0��˸
	}	
	LCD_ShowString(20+96,170,200,10,16,"AP3216C OK");  

	while(AT24CXX_Check())		//��ⲻ��24C02
	{
		LCD_ShowString(20,190,200,16,16,"24C02 Check Failed!");
		delay_ms(500);
		LCD_ShowString(20,190,200,16,16,"Please Check!        ");
		delay_ms(500);
		LED0_Toggle;				//DS0��˸
	}
	LCD_ShowString(20,190,200,16,16,"24C02   OK"); 
								
		while(W25QXX_ReadID()!=W25Q256)								//��ⲻ��W25Q256
	{
		LCD_ShowString(20,190,200,16,16,"QSPI Check Failed!");
		delay_ms(500);
		LCD_ShowString(20,190,200,16,16,"Please Check!        ");
		delay_ms(500);
		LED0_Toggle;		//DS0��˸
	}
	LCD_ShowString(20+96,190,200,16,16,"W25Q256 OK"); 
	
	POINT_COLOR=BLUE;			//��������Ϊ��ɫ	
	
    LCD_ShowString(20,210,200,16,16," IR:");	 
	LCD_ShowString(20,230,200,16,16," PS:");	
	LCD_ShowString(20,250,200,16,16,"ALS:");	 
	LCD_ShowString(20,270,200,6,16,"Dist: ");
	LCD_ShowString(20+72,270,200,6,16,"Time: ");
	LCD_ShowString(20+144,270,200,6,16,"Ligt: ");

	LCD_ShowString(20,290,200,16,16,"Alarm light:");
	LCD_ShowString(20,310,200,5,16,"time:");
	LCD_ShowString(20+114,310,200,5,16,"dist:");
	
	LCD_ShowString(20,350,200,16,16,"distance setting   <---");
	LCD_ShowString(20,370,200,16,16,"light setting          ");
	LCD_ShowString(20,390,200,16,16,"time  setting          ");
	

	//POINT_COLOR=GREEN;			//��������Ϊ��ɫ
	LCD_ShowString(20+40,270,200,3,16,"YES");
	LCD_ShowString(20+112,270,200,3,16,"NO ");
	LCD_ShowString(20+184,270,200,3,16,"NO ");
	
	#if SPISave 					
	W25QXX_Read(&mmm,0,sizeof(mmm));								//��W25Q256��ȡ
	W25QXX_Read(&time_value,2,sizeof(time_value));	
	W25QXX_Read(&dis_set,3,sizeof(dis_set));	
	#else
	mmm = AT24CXX_ReadLenByte(0,sizeof(mmm));					//��24C02��ȡ
	time_value = AT24CXX_ReadLenByte(125,sizeof(time_value));
	#endif
	
	while(1)
		{
			//time_value = m1 ;
			
			//(ALS:AmbientLight Sensor)��ǿ, (PS: Proximity Sensor)�ӽ�  LED(IR LED)����
			AP3216C_ReadData(&ir,&ps,&als);	//��ȡ���� 
			LCD_ShowNum(20+32,210,ir,5,16);	//��ʾIR����
			LCD_ShowNum(20+32,230,ps,5,16);	//��ʾPS����
			LCD_ShowNum(20+32,250,als,5,16);//��ʾALS���� 
			LCD_ShowxNum(20+100,290,mmm,3,16,0);//��ʾ����������ǿֵ
			LCD_ShowxNum(20+46,310,time_counter,3,16,0);//��ʾ��ǰʱ��ֵ
			LCD_ShowString(20+70,310,200,1,16,"/");	//��ʾʱ��ָ��
			LCD_ShowxNum(20+76,310,time_value*60,3,16,0);//��ʾ����ʱ��ֵ
			LCD_ShowString(20+192,310,200,1,16,"/");	//��ʾ����ָ��
			LCD_ShowxNum(20+208,310,dis_set ,3,16,0);//��ʾ���������
			//LCD_ShowxNum(20+96,410,y,3,16,0);
			//delay_ms(1000);
			
			
			if(setting == 1)
				{
					LCD_ShowString(20,350,200,16,16,"distance setting       ");
					LCD_ShowString(20,370,200,16,16,"light setting      <---");
					LCD_ShowString(20,390,200,16,16,"time  setting          ");
					//setting=1;
				}
			else if(setting == 0)
				{
					LCD_ShowString(20,350,200,16,16,"distance setting   <---");
					LCD_ShowString(20,370,200,16,16,"light setting          ");
					LCD_ShowString(20,390,200,16,16,"time  setting          ");
					//setting=0;
				}
			else if(setting == 2)
				{
					LCD_ShowString(20,350,200,16,16,"distance setting       ");
					LCD_ShowString(20,370,200,16,16,"light setting          ");
					LCD_ShowString(20,390,200,16,16,"time  setting      <---");
					//setting=0;
				}
			
			
			if(als<mmm||als>200) //��ǿ�������ǿ  ����ֵ�����Զ��� ��ǿָ��200
			{
				beepsta=!beepsta;					//������״̬ȡ��
				PCF8574_WriteBit(BEEP_IO,beepsta);	//���Ʒ�����
				LCD_ShowString(20+40,270,200,5,16,"YES");
				beepuse = 1;
			}
			else
			{
				if(!beepsta&&beepuse==1)						//�������������  ������ͣ��
				{
					beepsta=!beepsta;					//������״̬ȡ��
					PCF8574_WriteBit(BEEP_IO,beepsta);	//���Ʒ�����
					//LED1(1);
				}
				LCD_ShowString(20+40,270,200,5,16,"NO ");  //������δ�� �ҹ�ǿ���� ֻ��Ҫ��Ļ��ʾ�·�����״̬����
			}
				
			if(time_counter > (60*time_value)-1 ) //ʱ������Զ���ֵ ��������1�� 
			{
				beepsta=!beepsta;					//�򿪷�����
				PCF8574_WriteBit(BEEP_IO,beepsta);	
				LCD_ShowString(20+112,270,200,5,16,"YES");
				//LED1(0);
				delay_ms(1000);
				beepsta=!beepsta;					//�رշ�����
				PCF8574_WriteBit(BEEP_IO,beepsta);	
				LCD_ShowString(20+112,270,200,5,16,"NO ");  //������Ļ��ʱ��״̬
				//LED1(1);
				time_counter = 0;								//ʱ������ ���¼�ʱ
			}
			
			if(dis_value < dis_set ) //���������� �����Զ��� 
			{
				beepsta=!beepsta;					//������״̬ȡ��
				PCF8574_WriteBit(BEEP_IO,beepsta);	//���Ʒ�����
				LCD_ShowString(20+184,270,200,5,16,"YES");
				beepuse = 1;
			}
			else
			{
				if(!beepsta&&beepuse == 1)						//�������������  ������ͣ��
				{
					beepsta=!beepsta;					//������״̬ȡ��
					PCF8574_WriteBit(BEEP_IO,beepsta);	//���Ʒ�����
					//LED1(1);
				}
				LCD_ShowString(20+184,270,200,5,16,"NO ");  //������δ�� �ҹ�ǿ���� ֻ��Ҫ��Ļ��ʾ�·�����״̬����
			}

			
			key=KEY_Scan(0); 				//��ѯɨ�谴��
			
			if(key==KEY1_PRES)				//KEY1 ȷ������
			{
				
				if(setting==1)					//��������������
				{
					LCD_ShowString(20,350,200,16,16,"                       ");
					LCD_ShowString(20,370,200,16,16,"Enter light setting    ");
					LCD_ShowString(20,390,200,16,16,"                       ");
					key2 = -1;									//���keyֵ ׼�������˵����ܰ���
					while(key2 != KEY1_PRES)					//����ȷ����   ����һֱ�޸�
					{
						while(key2!=KEY0_PRES && key2 != KEY1_PRES && key2 != KEY2_PRES)		//ֻ����key0 key2 key1 ������� ��ѭ����
						{
							key2 = KEY_Scan(0);
						}				
						if(key2==KEY0_PRES)						//KEY0���� �������
						{ 
							if(mmm>0)							//��ֹ��Ϊ��
							{
								mmm-=5;
								LCD_ShowxNum(20+100,290,mmm,3,16,0);
							}
							key2 = -1;
						}
						if(key2==KEY2_PRES)						//KEY2���� ��������
						{
							key2 = -1;
							mmm+=5;
							LCD_ShowxNum(20+100,290,mmm,3,16,0);
						}
						
					}
					//SDRAM �е�ַ��8192 �����е�ַ��512 ����BANK ����4 ����λ�� 16 λ������������оƬ������Ϊ��8192*512*4*16=32M �ֽڡ� 
					#if SPISave 
					W25QXX_Write(&mmm,0,sizeof (mmm));					//д��W25Q256
					#else
					AT24CXX_WriteLenByte(0,mmm,sizeof(mmm));						//д��24C02
					#endif
					LCD_ShowString(20,350,200,16,16,"distance setting       ");
					LCD_ShowString(20,370,200,16,16,"light setting      <---");
					LCD_ShowString(20,390,200,16,16,"time  setting          ");
					continue ;
					
				}
				if(setting==0)		//�����˾�������
				{
					LCD_ShowString(20,350,200,16,16,"                       ");
					LCD_ShowString(20,370,200,16,16,"Enter distance setting ");
					LCD_ShowString(20,390,200,16,16,"                       ");
					LCD_ShowxNum(20+208,310,dis_set,3,16,0);//��ʾ����ʱ��ֵ
					key2 = -1;									//���keyֵ ׼�������˵����ܰ���
					while(key2 != KEY1_PRES)					//����ȷ����   ����һֱ�޸�
					{
						while(key2!=KEY0_PRES && key2 != KEY1_PRES && key2 != KEY2_PRES)		//ֻ����key0 key2 key1 ������� ��ѭ����
						{
							key2 = KEY_Scan(0);
						}				
						if(key2==KEY0_PRES)						//KEY0���� ʱ�����
						{ 
							if(dis_set >0)							//��ֹ��Ϊ0���Ϊ��
							{
								dis_set  -=5;
								LCD_ShowxNum(20+208,310,dis_set ,3,16,0);
							}
							key2 = -1;
						}
						if(key2==KEY2_PRES)						//KEY2���� ʱ������
						{
							key2 = -1;
							if(dis_set<400)							//��ֹ̫��
							{
							dis_set += 5;
							LCD_ShowxNum(20+208,310,dis_set ,3,16,0);
							}
						}
						
					}
					W25QXX_Write(&dis_set ,3,sizeof (dis_set ));					//д��W25Q256
					LCD_ShowString(20,350,200,16,16,"distance setting   <---");
					LCD_ShowString(20,370,200,16,16,"light setting          ");
					LCD_ShowString(20,390,200,16,16,"time  setting          ");
					continue ;
					
				}
				if(setting==2)		//������ʱ������
				{
					LCD_ShowString(20,350,200,16,16,"                       ");
					LCD_ShowString(20,370,200,16,16,"Enter time  setting    ");
					LCD_ShowString(20,390,200,16,16,"                       ");
					LCD_ShowxNum(20+76,310,time_value,3,16,0);//��ʾ����ʱ��ֵ
					key2 = -1;									//���keyֵ ׼�������˵����ܰ���
					while(key2 != KEY1_PRES)					//����ȷ����   ����һֱ�޸�
					{
						while(key2!=KEY0_PRES && key2 != KEY1_PRES && key2 != KEY2_PRES)		//ֻ����key0 key2 key1 ������� ��ѭ����
						{
							key2 = KEY_Scan(0);
						}				
						if(key2==KEY0_PRES)						//KEY0���� ʱ�����
						{ 
							if(time_value>1)							//��ֹ��Ϊ0���Ϊ��
							{
								time_value -=1;
								LCD_ShowxNum(20+76,310,time_value,3,16,0);
							}
							key2 = -1;
						}
						if(key2==KEY2_PRES)						//KEY2���� ʱ������
						{
							key2 = -1;
							if(time_value<15)							//��ֹ̫��
							{
							time_value += 1;
							LCD_ShowxNum(20+76,310,time_value,3,16,0);
							}
						}
						
					}
					#if SPISave 
					W25QXX_Write(&time_value,2,sizeof (time_value));					//д��W25Q256
					#else
					AT24CXX_WriteLenByte(125,time_value,sizeof(time_value));			//д��24C02
					#endif
					LCD_ShowString(20,350,200,16,16,"distance setting       ");
					LCD_ShowString(20,370,200,16,16,"light setting          ");
					LCD_ShowString(20,390,200,16,16,"time  setting      <---");
					continue ;
					
				}
				
					
			}
			else if(key==WKUP_PRES)	//һ���˵��л�����
			{
				if(setting == 0)
				{
					//LCD_ShowString(20,350,200,16,16,"distance setting       ");
					//LCD_ShowString(20,370,200,16,16,"time  setting      <---");
					setting=1;
				}
				else if(setting == 1)
				{
					//LCD_ShowString(20,350,200,16,16,"distance setting   <---");
					//LCD_ShowString(20,370,200,16,16,"time  setting          ");
					setting=2;
				}
				else if(setting == 2)
				{
					//LCD_ShowString(20,350,200,16,16,"distance setting   <---");
					//LCD_ShowString(20,370,200,16,16,"time  setting          ");
					setting=0;
				}
			}
			
			//��������
			LCD_DrawRectangle(15,410,112,470);
			LCD_ShowString(38,430,200,16,16,"Switch");		//��ʾ��ʾ��Ϣ		
			LCD_DrawRectangle(135,410,232,470);
			LCD_ShowString(38+120,430,200,16,16,"Enter ");		//��ʾ��ʾ��Ϣ		
			ctp_test();
				
			//����������
			
			WPA5(1); //PA5
			//LED1(1);
			HAL_Delay(20);
			WPA5(0);
			//LED1(0);
			//HAL_Delay(100); 
			delay_ms(5);
			if(tim != 0)
			{
				dis_value  = (((50000-tim)*17)/10);			//���ݶ�ȡֵ �������
			}
			
			delay_ms(5);
			if(i%5==0)
			{				
				LCD_ShowxNum(30+142,310,dis_value,5,16,0);							//��ʾ���� 
			}			
			if(PCF8574_INT==0)				//PCF8574���жϵ͵�ƽ��Ч
			{
				key=PCF8574_ReadBit(EX_IO);	//��ȡEXIO״̬,ͬʱ���PCF8574���ж����(INT�ָ��ߵ�ƽ)
				if(key==0)LED1_Toggle;		//LED1״̬ȡ�� 
			}
			i++;
			//delay_ms(10);
			
			
			
			if(i==25)
			{
				
				LED0_Toggle;//��ʾϵͳ��������	
				i=0;
			}		   
		} 	     
}
