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

#define SPISave 1			//实现掉电保存的方式  为1时，使用SPI读写W25Q256  为0时 使用I2C读写24C02

// 超声波Trig接PA5  Echo接PD3  

int set_flag = 0;			//按键中断用的全局变量 暂时停用
u16 tim;					//用于保存超声波模块定时器的值

unsigned long int time_counter = 0;		//定时器中断计时用的全局变量 代表秒数
int setting = 0;						//菜单功能标志位

	u8 t=0;
	u8 i=0;	  	    
 	u16 lastpos[10][2];		//最后一次的数据 
	u8 maxp=5;


//清空屏幕并在右上角显示"RST"
void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);//清屏   
 	POINT_COLOR=BLUE;//设置字体为蓝色 
	LCD_ShowString(lcddev.width-24,0,200,16,16,"RST");//显示清屏区域
  	POINT_COLOR=RED;//设置画笔蓝色 
}
////////////////////////////////////////////////////////////////////////////////
//电容触摸屏专有部分
//画水平线
//x0,y0:坐标
//len:线长度
//color:颜色
void gui_draw_hline(u16 x0,u16 y0,u16 len,u16 color)
{
	if(len==0)return;
	LCD_Fill(x0,y0,x0+len-1,y0,color);	
}
//画实心圆
//x0,y0:坐标
//r:半径
//color:颜色
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
//两个数之差的绝对值 
//x1,x2：需取差值的两个数
//返回值：|x1-x2|
u16 my_abs(u16 x1,u16 x2)
{			 
	if(x1>x2)return x1-x2;
	else return x2-x1;
}  
//画一条粗线
//(x1,y1),(x2,y2):线条的起始坐标
//size：线条的粗细程度
//color：线条的颜色
void lcd_draw_bline(u16 x1, u16 y1, u16 x2, u16 y2,u8 size,u16 color)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	if(x1<size|| x2<size||y1<size|| y2<size)return; 
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		gui_fill_circle(uRow,uCol,size,color);//画点 
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
//10个触控点的颜色(电容触摸屏用)												 
const u16 POINT_COLOR_TBL[10]={RED,GREEN,BLUE,BROWN,GRED,BRED,GBLUE,LIGHTBLUE,BRRED,GRAY};  
//电阻触摸屏测试函数
void rtp_test(void)
{
	u8 key;
	u8 i=0;	  
	while(1)
	{
	 	key=KEY_Scan(0);
		tp_dev.scan(0); 		 
		if(tp_dev.sta&TP_PRES_DOWN)			//触摸屏被按下
		{	
		 	if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.x[0]>(lcddev.width-24)&&tp_dev.y[0]<16)Load_Drow_Dialog();//清除
				else TP_Draw_Big_Point(tp_dev.x[0],tp_dev.y[0],RED);		//画图	  			   
			}
		}else delay_ms(10);	//没有按键按下的时候 	    
		if(key==KEY0_PRES)	//KEY0按下,则执行校准程序
		{
			LCD_Clear(WHITE);	//清屏
		    TP_Adjust();  		//屏幕校准 
			TP_Save_Adjdata();	 
			Load_Drow_Dialog();
		}
		i++;
		if(i%20==0)LED0_Toggle;
	}
}
//电容触摸屏测试函数
void ctp_test(void)
{
	//u8 t=0;
	//u8 i=0;	  	    
 	//u16 lastpos[10][2];		//最后一次的数据 
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
					//lcd_draw_bline(lastpos[t][0],lastpos[t][1],tp_dev.x[t],tp_dev.y[t],2,POINT_COLOR_TBL[t]);//画线
					lastpos[t][0]=tp_dev.x[t];
					lastpos[t][1]=tp_dev.y[t];
					if(15<tp_dev.x[t]&&tp_dev.x[t]<112&&410<tp_dev.y[t]&&tp_dev.y[t]<470)
					{
						//Load_Drow_Dialog();//清除
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
						//Load_Drow_Dialog();//清除
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
    Cache_Enable();                 //打开L1-Cache
    HAL_Init();				        //初始化HAL库
    Stm32_Clock_Init(432,25,2,9);   //设置时钟,216Mhz 
    delay_init(216);                //延时初始化
	uart_init(115200);		        //串口初始化
    LED_Init();                     //初始化LED
	
    KEY_Init();                     //初始化按键
    SDRAM_Init();                   //初始化SDRAM
    LCD_Init();                     //LCD初始化
	W25QXX_Init();		            //初始化W25QXX
	EXTI_Init();					//中断初始化 暂时停用
	TIM3_Init(10000-1,10800-1);      //系统注释：定时器3初始化，定时器时钟为108M，分频系数为10800-1，
									 //系统注释：所以定时器3的频率为108M/10800=10K，自动重装载为5000-1，那么定时器周期就是500ms
									 //50000-1 5秒一中断  10000-1 1秒一中断
									 //定时器3用于系统计时
	TIM4_Init(50000-1,10800-1); 	 //定时器4用于超声波模块计算差值
	tp_dev.init();				    //触摸屏初始化 
	//HAL_TIM_Base_Start_IT(&TIM4_Handler);
	
	//if(tp_dev.touchtype!=0XFF)LCD_ShowString(30,130,200,16,16,"Press KEY0 to Adjust");//电阻屏才显示
	//delay_ms(1500);
 	//Load_Drow_Dialog();	 	
	
	//if(tp_dev.touchtype&0X80)ctp_test();//电容屏测试
	//else rtp_test(); 					//电阻屏测试  									  	       

	
	POINT_COLOR=RED;
	LCD_ShowString(20,30,200,32,16,"Vision protector");	
	LCD_ShowString(20,50,200,16,16,"MuZhi@xsyu 2019/6/18"); 
	
	LCD_ShowString(20,90,200,16,16,"KEY2/KEY0: Add/Less");			//显示提示信息	
	LCD_ShowString(20,110,200,16,16,"KEY_UP: Switch");		//显示提示信息		
	LCD_ShowString(20,130,200,16,16,"KEY1: Enter");		//显示提示信息		
	
	while(PCF8574_Init())		//检测不到PCF8574
	{
		LCD_ShowString(20,170,200,16,16,"PCF8574 Check Failed!");
		delay_ms(500);
		LCD_ShowString(20,170,200,16,16,"Please Check!      ");
		delay_ms(500);
		LED0_Toggle;//DS0闪烁
	}
	LCD_ShowString(20,170,200,16,16,"PCF8574 OK");    
	//POINT_COLOR=BLUE;//设置字体为蓝色
	
	while(AP3216C_Init())		//检测不到AP3216C
	{
		LCD_ShowString(20,170,200,16,16,"AP3216C Check Failed!");
		delay_ms(500);
		LCD_ShowString(20,170,200,16,16,"Please Check!        ");
		delay_ms(500);
		LED0_Toggle;				//DS0闪烁
	}	
	LCD_ShowString(20+96,170,200,10,16,"AP3216C OK");  

	while(AT24CXX_Check())		//检测不到24C02
	{
		LCD_ShowString(20,190,200,16,16,"24C02 Check Failed!");
		delay_ms(500);
		LCD_ShowString(20,190,200,16,16,"Please Check!        ");
		delay_ms(500);
		LED0_Toggle;				//DS0闪烁
	}
	LCD_ShowString(20,190,200,16,16,"24C02   OK"); 
								
		while(W25QXX_ReadID()!=W25Q256)								//检测不到W25Q256
	{
		LCD_ShowString(20,190,200,16,16,"QSPI Check Failed!");
		delay_ms(500);
		LCD_ShowString(20,190,200,16,16,"Please Check!        ");
		delay_ms(500);
		LED0_Toggle;		//DS0闪烁
	}
	LCD_ShowString(20+96,190,200,16,16,"W25Q256 OK"); 
	
	POINT_COLOR=BLUE;			//设置字体为蓝色	
	
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
	

	//POINT_COLOR=GREEN;			//设置字体为绿色
	LCD_ShowString(20+40,270,200,3,16,"YES");
	LCD_ShowString(20+112,270,200,3,16,"NO ");
	LCD_ShowString(20+184,270,200,3,16,"NO ");
	
	#if SPISave 					
	W25QXX_Read(&mmm,0,sizeof(mmm));								//从W25Q256读取
	W25QXX_Read(&time_value,2,sizeof(time_value));	
	W25QXX_Read(&dis_set,3,sizeof(dis_set));	
	#else
	mmm = AT24CXX_ReadLenByte(0,sizeof(mmm));					//从24C02读取
	time_value = AT24CXX_ReadLenByte(125,sizeof(time_value));
	#endif
	
	while(1)
		{
			//time_value = m1 ;
			
			//(ALS:AmbientLight Sensor)光强, (PS: Proximity Sensor)接近  LED(IR LED)红外
			AP3216C_ReadData(&ir,&ps,&als);	//读取数据 
			LCD_ShowNum(20+32,210,ir,5,16);	//显示IR数据
			LCD_ShowNum(20+32,230,ps,5,16);	//显示PS数据
			LCD_ShowNum(20+32,250,als,5,16);//显示ALS数据 
			LCD_ShowxNum(20+100,290,mmm,3,16,0);//显示报警最弱光强值
			LCD_ShowxNum(20+46,310,time_counter,3,16,0);//显示当前时间值
			LCD_ShowString(20+70,310,200,1,16,"/");	//显示时间分割符
			LCD_ShowxNum(20+76,310,time_value*60,3,16,0);//显示报警时间值
			LCD_ShowString(20+192,310,200,1,16,"/");	//显示距离分割符
			LCD_ShowxNum(20+208,310,dis_set ,3,16,0);//显示报警距离符
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
			
			
			if(als<mmm||als>200) //光强过弱或过强  过弱值允许自定义 过强指定200
			{
				beepsta=!beepsta;					//蜂鸣器状态取反
				PCF8574_WriteBit(BEEP_IO,beepsta);	//控制蜂鸣器
				LCD_ShowString(20+40,270,200,5,16,"YES");
				beepuse = 1;
			}
			else
			{
				if(!beepsta&&beepuse==1)						//如果蜂鸣器在响  蜂鸣器停掉
				{
					beepsta=!beepsta;					//蜂鸣器状态取反
					PCF8574_WriteBit(BEEP_IO,beepsta);	//控制蜂鸣器
					//LED1(1);
				}
				LCD_ShowString(20+40,270,200,5,16,"NO ");  //蜂鸣器未响 且光强正常 只需要屏幕显示下蜂鸣器状态就行
			}
				
			if(time_counter > (60*time_value)-1 ) //时间大于自定义值 则长鸣报警1秒 
			{
				beepsta=!beepsta;					//打开蜂鸣器
				PCF8574_WriteBit(BEEP_IO,beepsta);	
				LCD_ShowString(20+112,270,200,5,16,"YES");
				//LED1(0);
				delay_ms(1000);
				beepsta=!beepsta;					//关闭蜂鸣器
				PCF8574_WriteBit(BEEP_IO,beepsta);	
				LCD_ShowString(20+112,270,200,5,16,"NO ");  //更改屏幕定时器状态
				//LED1(1);
				time_counter = 0;								//时间清零 重新计时
			}
			
			if(dis_value < dis_set ) //超声波距离 允许自定义 
			{
				beepsta=!beepsta;					//蜂鸣器状态取反
				PCF8574_WriteBit(BEEP_IO,beepsta);	//控制蜂鸣器
				LCD_ShowString(20+184,270,200,5,16,"YES");
				beepuse = 1;
			}
			else
			{
				if(!beepsta&&beepuse == 1)						//如果蜂鸣器在响  蜂鸣器停掉
				{
					beepsta=!beepsta;					//蜂鸣器状态取反
					PCF8574_WriteBit(BEEP_IO,beepsta);	//控制蜂鸣器
					//LED1(1);
				}
				LCD_ShowString(20+184,270,200,5,16,"NO ");  //蜂鸣器未响 且光强正常 只需要屏幕显示下蜂鸣器状态就行
			}

			
			key=KEY_Scan(0); 				//轮询扫描按键
			
			if(key==KEY1_PRES)				//KEY1 确定按键
			{
				
				if(setting==1)					//进入了亮度设置
				{
					LCD_ShowString(20,350,200,16,16,"                       ");
					LCD_ShowString(20,370,200,16,16,"Enter light setting    ");
					LCD_ShowString(20,390,200,16,16,"                       ");
					key2 = -1;									//清空key值 准备二级菜单接受按键
					while(key2 != KEY1_PRES)					//除非确定键   否则一直修改
					{
						while(key2!=KEY0_PRES && key2 != KEY1_PRES && key2 != KEY2_PRES)		//只接受key0 key2 key1 如果不是 死循环等
						{
							key2 = KEY_Scan(0);
						}				
						if(key2==KEY0_PRES)						//KEY0按下 距离减少
						{ 
							if(mmm>0)							//防止减为负
							{
								mmm-=5;
								LCD_ShowxNum(20+100,290,mmm,3,16,0);
							}
							key2 = -1;
						}
						if(key2==KEY2_PRES)						//KEY2按下 距离增加
						{
							key2 = -1;
							mmm+=5;
							LCD_ShowxNum(20+100,290,mmm,3,16,0);
						}
						
					}
					//SDRAM 行地址：8192 个；列地址：512 个；BANK 数：4 个；位宽： 16 位；这样，整个芯片的容量为：8192*512*4*16=32M 字节。 
					#if SPISave 
					W25QXX_Write(&mmm,0,sizeof (mmm));					//写入W25Q256
					#else
					AT24CXX_WriteLenByte(0,mmm,sizeof(mmm));						//写入24C02
					#endif
					LCD_ShowString(20,350,200,16,16,"distance setting       ");
					LCD_ShowString(20,370,200,16,16,"light setting      <---");
					LCD_ShowString(20,390,200,16,16,"time  setting          ");
					continue ;
					
				}
				if(setting==0)		//进入了距离设置
				{
					LCD_ShowString(20,350,200,16,16,"                       ");
					LCD_ShowString(20,370,200,16,16,"Enter distance setting ");
					LCD_ShowString(20,390,200,16,16,"                       ");
					LCD_ShowxNum(20+208,310,dis_set,3,16,0);//显示报警时间值
					key2 = -1;									//清空key值 准备二级菜单接受按键
					while(key2 != KEY1_PRES)					//除非确定键   否则一直修改
					{
						while(key2!=KEY0_PRES && key2 != KEY1_PRES && key2 != KEY2_PRES)		//只接受key0 key2 key1 如果不是 死循环等
						{
							key2 = KEY_Scan(0);
						}				
						if(key2==KEY0_PRES)						//KEY0按下 时间减少
						{ 
							if(dis_set >0)							//防止减为0或减为负
							{
								dis_set  -=5;
								LCD_ShowxNum(20+208,310,dis_set ,3,16,0);
							}
							key2 = -1;
						}
						if(key2==KEY2_PRES)						//KEY2按下 时间增加
						{
							key2 = -1;
							if(dis_set<400)							//防止太大
							{
							dis_set += 5;
							LCD_ShowxNum(20+208,310,dis_set ,3,16,0);
							}
						}
						
					}
					W25QXX_Write(&dis_set ,3,sizeof (dis_set ));					//写入W25Q256
					LCD_ShowString(20,350,200,16,16,"distance setting   <---");
					LCD_ShowString(20,370,200,16,16,"light setting          ");
					LCD_ShowString(20,390,200,16,16,"time  setting          ");
					continue ;
					
				}
				if(setting==2)		//进入了时间设置
				{
					LCD_ShowString(20,350,200,16,16,"                       ");
					LCD_ShowString(20,370,200,16,16,"Enter time  setting    ");
					LCD_ShowString(20,390,200,16,16,"                       ");
					LCD_ShowxNum(20+76,310,time_value,3,16,0);//显示报警时间值
					key2 = -1;									//清空key值 准备二级菜单接受按键
					while(key2 != KEY1_PRES)					//除非确定键   否则一直修改
					{
						while(key2!=KEY0_PRES && key2 != KEY1_PRES && key2 != KEY2_PRES)		//只接受key0 key2 key1 如果不是 死循环等
						{
							key2 = KEY_Scan(0);
						}				
						if(key2==KEY0_PRES)						//KEY0按下 时间减少
						{ 
							if(time_value>1)							//防止减为0或减为负
							{
								time_value -=1;
								LCD_ShowxNum(20+76,310,time_value,3,16,0);
							}
							key2 = -1;
						}
						if(key2==KEY2_PRES)						//KEY2按下 时间增加
						{
							key2 = -1;
							if(time_value<15)							//防止太大
							{
							time_value += 1;
							LCD_ShowxNum(20+76,310,time_value,3,16,0);
							}
						}
						
					}
					#if SPISave 
					W25QXX_Write(&time_value,2,sizeof (time_value));					//写入W25Q256
					#else
					AT24CXX_WriteLenByte(125,time_value,sizeof(time_value));			//写入24C02
					#endif
					LCD_ShowString(20,350,200,16,16,"distance setting       ");
					LCD_ShowString(20,370,200,16,16,"light setting          ");
					LCD_ShowString(20,390,200,16,16,"time  setting      <---");
					continue ;
					
				}
				
					
			}
			else if(key==WKUP_PRES)	//一级菜单切换按键
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
			
			//触摸部分
			LCD_DrawRectangle(15,410,112,470);
			LCD_ShowString(38,430,200,16,16,"Switch");		//显示提示信息		
			LCD_DrawRectangle(135,410,232,470);
			LCD_ShowString(38+120,430,200,16,16,"Enter ");		//显示提示信息		
			ctp_test();
				
			//超声波部分
			
			WPA5(1); //PA5
			//LED1(1);
			HAL_Delay(20);
			WPA5(0);
			//LED1(0);
			//HAL_Delay(100); 
			delay_ms(5);
			if(tim != 0)
			{
				dis_value  = (((50000-tim)*17)/10);			//根据读取值 计算距离
			}
			
			delay_ms(5);
			if(i%5==0)
			{				
				LCD_ShowxNum(30+142,310,dis_value,5,16,0);							//显示距离 
			}			
			if(PCF8574_INT==0)				//PCF8574的中断低电平有效
			{
				key=PCF8574_ReadBit(EX_IO);	//读取EXIO状态,同时清除PCF8574的中断输出(INT恢复高电平)
				if(key==0)LED1_Toggle;		//LED1状态取反 
			}
			i++;
			//delay_ms(10);
			
			
			
			if(i==25)
			{
				
				LED0_Toggle;//提示系统正在运行	
				i=0;
			}		   
		} 	     
}
