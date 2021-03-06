/*
大创项目：智能蓝牙风扇
创建时间2019.9.28，12.24
创建人：韦达
团队原创

智能蓝牙风扇实现功能如下：
1、蓝牙控制数据传输      PA9 PA10
2、按键控制智能模式切换  PB5
3、按键控制转速、定时    PB6、PB7
4、自动检测无人环境 	    PA4
5、DHT11环境温湿度检查   PB11
6、OLED显示        	    PCout(14)//SCL   串行时钟 PCout(15)//SDA	
7、声光报警装置：		BEEP：  P12        LED2：PB13(红)、14(绿色)、15(蓝色)
*/
#include "sys.h"
#include "delay.h"
#include "usart.h"	
#include "exti.h"
#include "led.h"    	/*灯*/
#include "key.h"	/*按键*/ 
#include "oled.h"    /*屏幕*/
#include "dht11.h"	/*温湿度*/
#include "pwm.h"	/*风扇PWM*/
#include "stdlib.h"

u32 time=0;					  	  /*用来定时的时间，单位：秒*/
DHT11_Data_TypeDef DHT11_Data;    /*温湿度数据结构体*/
u8  temperature,humidity;  	  	  /*DHT11温度与湿度*/
u8  Interface=0,Interface1=0;     /*界面切换变量*/
u8  add=0,add1=0;    			/*电机切换挡位变量*/
u16 receive=0;  		 		/*蓝牙接收回来的数据*/
u16 Speed=0;     		 		/*电机转速标志量*/  
u16 buff=0;  				    /*百分比基础数量*/
u32 User_Time=0; 	 			/*用户定时变量*/
u8  Open_time=0; 				 /*1的时候是没有人启动定时器关闭风扇，2的时候是蓝牙控制定时时间，3是传统定时*/
u8  temp_up=0,temp_dowm=0;  	  /*用户传回来关机温度*/
u8  Receive_Buff[USART_REC_LEN];  /*接收缓存数组*/
u8  timeBuff[3]; 	/*时间缓存数组*/
u8  send_flag=0;  	/*DHT11发送数据标志量*/
u8  zhuansu=0;	 	//温控转速判断
u16 Sleep_pwmval=0; /*睡眠控制的PWM变量*/
u8 dir=1,dir1=0;	         /*睡眠控制是否工作变量*/
u8 temperature_buff[2],Intelligence_time=0;//智能模式下的缓存数组
u8 time_DHT11=0,time1_DHT11=0,read_IO;//DHT11启动子程序  read_IO读取红外检测IO变量

//界面初始化
void Init_face(){

	/*初始化界面*/
	OLED_P6x8Str(9,1,"Mode : ");
	OLED_P6x8Str(9,3,"Time : ");
	OLED_P6x8Str(9,5,"Temp : ");
	OLED_P6x8Str(9,7,"water: ");
	OLED_P6x8Str(9+65,0,"Speed:");
	OLED_ShowChar(9+45+18,2,'H');	//显示小时
	OLED_ShowChar(9+75+18,2,'M');	//显示分钟
	OLED_ShowChar(69,4,'.');	    //显示小数点	
	OLED_ShowChar(69,6,'.');	    //显示小数点	
	OLED_ShowChar(90,4,'C');	    //显示C
	OLED_ShowChar(90,6,'%');	    //显示%	
}

/*模式切换指示灯*/
void LED_change(){

		LED_Green=!LED_Green;  /*模式切换指示灯*/
		delay_ms(500);
		LED_Green=!LED_Green;
		delay_ms(500);
}

/*成功接收指示灯闪烁*/
void LED_Succeed(){

		LED_Blue=!LED_Blue;  /*模式切换指示灯*/
		delay_ms(100);
		LED_Blue=!LED_Blue;
		delay_ms(100);
		LED_Blue=!LED_Blue;  /*模式切换指示灯*/
		delay_ms(100);
		LED_Blue=!LED_Blue;
		delay_ms(100);
		LED_Blue=0;
	
}

/*定时关闭提醒*/
void LED_Close(){
	 BEEP=!BEEP;
	 LED_RED=!LED_RED;            //没人存在的时候红灯报警
	 delay_ms(200);
	 BEEP=!BEEP;
	 LED_RED=!LED_RED;            //没人存在的时候红灯报�
	 delay_ms(200);
}

//智能模式
void Intelligence_Mode(){

	/*1、随着温度改变PWM输出 15-40度 PWM范围：
	   2、间歇性吹风*/
	Intelligence_time++;
	delay_ms(30);
	if(Intelligence_time%100==0)  //每3S读取一次
	{
				Intelligence_time=0;
				//DHT11_Read_Data(&temperature,&humidity);	//读取温湿度值
				temperature_buff[0]	= temperature;			/*加载缓存*/
				
				OLED_ShowNum(65,4,temperature,2,16);	    //显示温度	
				OLED_ShowNum(65,6,humidity,2,16);				//显示湿度	
				
				//速度随着温度变化改变PWM
		
				if(temperature_buff[0]==15){
						Speed =0;
						OLED_ShowNum(9+100,0,Speed,2,16); //速度
						TIM_SetCompare1(TIM3,0);
				}
				if(temperature_buff[0]<15){
						Speed = 1100/32;
						OLED_ShowNum(9+100,0,Speed,2,16); //速度
						TIM_SetCompare1(TIM3,1100);
				}
				
				if(temperature_buff[0]>15&&temperature_buff[0]>20){
					   Speed = 1200/32;
						OLED_ShowNum(9+100,0,Speed,2,16); //速度
						TIM_SetCompare1(TIM3,1200); 
				}
				
				if(temperature_buff[0]>20&&temperature_buff[0]>25){
					    Speed = 1250/32;
						OLED_ShowNum(9+100,0,Speed,2,16); //速度						
						TIM_SetCompare1(TIM3,1250);  
				}
				
				if(temperature_buff[0]>25&&temperature_buff[0]>30){
						Speed = 1300/32;
						OLED_ShowNum(9+100,0,Speed,2,16); //速度
						TIM_SetCompare1(TIM3,1300); 
				}
				
				if(temperature_buff[0]>30&&temperature_buff[0]>35){
						Speed = 1380/32;
						OLED_ShowNum(9+100,0,Speed,2,16); //速度
						TIM_SetCompare1(TIM3,1380);   
				}

				if(temperature_buff[0]>35){
						Speed = 1420/32;
						OLED_ShowNum(9+100,0,Speed,2,16); //速度
						TIM_SetCompare1(TIM3,1420);  
				}
	}
}

//按键控制转速、定时  PB6、PB7
void Shift_PWM(){

	/*PB6按下：更换挡位*/ 
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_6)==0)
	{
		delay_ms(120);
		if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_6)==0)
		{			
			add+=1;
			if(add>3)add=0;
		}
	}
	
	/*PB7按下：传统定时按钮*/ 
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)==0){
		delay_ms(120);
		if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)==0)
		{			
			/*传统定时每次按下多十分钟，只能1-3小时*/
			User_Time+=10;
			if(User_Time<60)
			{
				TIM_Cmd(TIM2, ENABLE);
				Open_time=3;
				timeBuff[1]=0;
				timeBuff[2]=User_Time;
			}
			
			if(User_Time>60&&User_Time<120){
				TIM_Cmd(TIM2, ENABLE);
				Open_time=3;
				timeBuff[1]=1;
				timeBuff[2]=60-(120-User_Time);
			}
			
			if(User_Time>120&&User_Time<180){
				TIM_Cmd(TIM2, ENABLE);
				Open_time=3;
				timeBuff[1]=2;
				timeBuff[2]=60-(180-User_Time);
			}
			
			switch(User_Time){
				case 60:
				TIM_Cmd(TIM2, ENABLE);
				Open_time=3;
				timeBuff[1]=1;
				timeBuff[2]=0;break;
				case 120:
				TIM_Cmd(TIM2, ENABLE);
				Open_time=3;
				timeBuff[1]=2;
				timeBuff[2]=0;break;
				case 180:
				TIM_Cmd(TIM2, ENABLE);
				Open_time=3;
				timeBuff[1]=3;
				timeBuff[2]=0;break;
			}
			
			if(User_Time>180){
				TIM_Cmd(TIM2, DISABLE);
				User_Time=0;
				timeBuff[1]=0;
				timeBuff[2]=0;
			}	
			
			OLED_ShowNum(9+45,2,timeBuff[1],2,16); //小时
			OLED_ShowNum(9+75,2,timeBuff[2],2,16); //分钟
			printf("定时时间： %dH,%dmin\r\n",timeBuff[1],timeBuff[2]);					
		}
	}
	
	//0的时候不动，1的时候一档，2的时候二档，3的时候三档
	if(add==0)
	{
		if(add1==0){
			add1=1;
			Speed = 0;
			OLED_ShowNum(9+100,0,Speed,2,16); //速度
			TIM_SetCompare1(TIM3,Speed);  	  //不动
			printf("关机\r\n");					
		}
	}
	//一档
	if(add==1)
	{
		if(add1==1){
			add1=2;
			Speed = 30;
			OLED_ShowNum(9+100,0,Speed,2,16); //速度
			TIM_SetCompare1(TIM3,Speed*38);   //一档
			printf("一档\r\n");						
		}
	}
	//二档
	if(add==2)
	{
		if(add1==2){
			add1=3;
			Speed = 60;
			OLED_ShowNum(9+100,0,Speed,2,16); //速度
			TIM_SetCompare1(TIM3,Speed*21);   //二档
			printf("二档\r\n");				
		}
	}
	//三档
	if(add==3)
	{
		if(add1==3){
			add1=0;
			Speed = 90;
			OLED_ShowNum(9+100,0,Speed,2,16); //速度
			TIM_SetCompare1(TIM3,Speed*15.5);  //三档
			printf("三档\r\n");	
		}
	}
}

void Read_Peopel(){

	//检测是否有人读取红外脚的值
		read_IO=GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4);
		//LED1=0;  	 	 /*LED=0是亮的意思*/ 
		if(read_IO==0){
			LED_RED=0;   	 /*低电平的时候是没有人的，高电平是有人的，没人的时候默认关闭*/
			Open_time=1;/*打开判断定时器工作变量为1，Open_time=1表示此时无人*/
			TIM_Cmd(TIM2, ENABLE);  //没人时候打开定时器		
			printf("现在无人\r\n");								
		}
		else{
			//智能模式下如果已经开启定时模式，有人的情况下也要打开定时器
			if(Open_time==2)TIM_Cmd(TIM2, ENABLE);   	    //如果蓝牙已经定时器
			
			else if(Open_time==3)TIM_Cmd(TIM2, ENABLE);   //如果传统已经定时
			
			else TIM_Cmd(TIM2, DISABLE); 		     	  //要是上面两种情况都不属于的时候关闭定时器
			LED_RED=1;  				 	 			  /*有人的时候LED=0开始亮起来*/
			printf("现在有人\r\n");
		}	
}

//DHT11数据上传
void DHT11_Start(){

	if(send_flag==1)
		{
			Read_DHT11(&DHT11_Data);//读取温湿度值	
			//DHT11_Read_Data(&temperature,&humidity);					    
			OLED_ShowNum(65,4,DHT11_Data.temp_int,2,16);		//显示温度	
			OLED_ShowNum(65,6,DHT11_Data.humi_int,2,16);		//显示湿度
			USART_SendData(USART1,0x52);  					//检测到0x52才算开始
			delay_ms(100);  
			USART_SendData(USART1,DHT11_Data.temp_int); //发送温度的十六进制
			delay_ms(1000);
			USART_SendData(USART1,DHT11_Data.humi_int);	//发送湿度的十六进制
			delay_ms(100);
			USART_SendData(USART1,0x53);//检测到0x53才算结束
			delay_ms(100);
		if(zhuansu==1)
		{
			if(temperature>temp_up)  /*超过温度上限*/
			{
				TIM_SetCompare1(TIM3,1500);
			}
			
			if(temperature<temp_dowm) /*超过温度下限*/
			{
				TIM_SetCompare1(TIM3,1100);
			}
		}
	}
		//	printf("现在温度为：%d°\r\n",temperature);	
		//	printf("现在湿度为：%d%%\r\n",humidity);
}

//自动检测函数 DHT11显示1min
void Check(){
	delay_ms(10);
	time_DHT11++;   //每次增加10ms，一共要增加一百次 10ms*10 = 100ms
	if(time_DHT11==15)
	{
		time_DHT11=0; /*等于0再来*/
		time1_DHT11++;
		//time_DHT11=10，time1_DHT11=120 这个定时是45S
		if(time1_DHT11==200)  // time1_DHT11=200，time_DHT11==15得到116S  差不多两分钟
		{
			time1_DHT11=0;
			send_flag=1;       /*DHT11确定工作变量*/
			Read_Peopel();  //读取红外是否检测到人
			DHT11_Start();	//DHT11温度显示
			send_flag=0;         /*DHT11关闭工作变量*/
		}	
	}
}

//睡眠模式
void Sleep_Mode(){
		//间歇性吹风
		Intelligence_time++;
		if(Intelligence_time%100==0)  //每3S读取一次
		{
					//间歇性吹风
					//确定温度差值
					Read_DHT11(&DHT11_Data);//读取温湿度值					    
					OLED_ShowNum(65,4,DHT11_Data.temp_int,2,16);		//显示温度	
					OLED_ShowNum(65,6,DHT11_Data.humi_int,2,16);		//显示湿度
					temperature_buff[0]	= DHT11_Data.temp_int;/*加载缓存*/
		}
		if(temperature_buff[0]<15)dir1=1;
		if(temperature_buff[0]>35)dir1=2;
		if(temperature_buff[0]>15&&temperature_buff[0]<35)dir1=0;
		switch(dir1){
			case 0:			
			delay_ms(50);	 
			if(dir)Sleep_pwmval+=50;
			else Sleep_pwmval-=50;	
			TIM_SetCompare1(TIM3,Sleep_pwmval);
			if(Sleep_pwmval>1400)
			{
			TIM_SetCompare1(TIM3,1400);
			delay_ms(5000);
			dir=0;		
			}			
			if(Sleep_pwmval==0)
			{	
			TIM_SetCompare1(TIM3,Sleep_pwmval);
			delay_ms(5000);
			dir=1;	
			}break;

			case 1:	TIM_SetCompare1(TIM3,0);break;
			case 2:	TIM_SetCompare1(TIM3,1500);break;
		}	
}

	
//蓝牙控制子程序
void Bluetooth(){
//传统模式、智能模式和睡眠模式切换
	//备注：一定要记得是0D 0A的数据帧尾，不然识别出来的数据不够完整
	u8 len,t;
	if(USART_RX_STA&0x8000)
	{				
			/*二进制	     十进制	十六进制 	图形
			  0011 0000	   48	    30	     0*/
			len=USART_RX_STA&0x3fff;//得到此次接收到的数据长度
			//printf("\r\n您发送的消息为:\r\n\r\n");
			for(t=0;t<len;t++)
			{
				Receive_Buff[t]=USART_RX_BUF[t];
				//USART_SendData(USART1, USART_RX_BUF[t]);//向串口1发送数据
				//while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
			}
			
			OLED_ShowNum(9+45,0,Receive_Buff[0],2,16);	  /*显示本次接受模式*/
			//蓝牙分支控制
			switch(Receive_Buff[0])
			{	
				//0x01:打开风扇电机;
				case 1:TIM_SetCompare1(TIM3,1200);break;
				
				//0x02:关闭风扇电机
				case 2:TIM_SetCompare1(TIM3,0); TIM_Cmd(TIM2, DISABLE);send_flag=0;break;
				
				//0x03:查看DHT11温湿度
				case 3:DHT11_Start();send_flag=1;break; 
				
				//0x10:返回按钮不查看
				case 10:send_flag=0;break; 
				
				//0x04:风扇控制传统电机转速1-3档
				case 4:add+=1;if(add>3)add=0;break; 
				
				/*0x05: 风扇控制百分比电机转速 0-99%
				  百分比控制电机转速 Receive_Buff[1]的参考范围为：0-99
				  参数一：0x05，参数二：手机端传来的百分数*/
				case 5:				
				if(Receive_Buff[1]==0){
					buff=0;/*输出0*/
					Speed=0;
				}
				else {
				Speed=Receive_Buff[1];
				OLED_ShowNum(9+100,0,Speed,2,16); 	//速度//显示速度百分比 
				buff = (Receive_Buff[1]*1.5)+1200;  /*控制百分比*/
				}
				TIM_SetCompare1(TIM3,buff);break;
				
				/*
				参数1：0x06,参数2：温度上限，参数3：温度下限
				温度上限就一直最高转速，下限就是关闭风扇**/
				case 6:Open_time=2;
					   temp_up   = Receive_Buff[1];
					   temp_dowm = Receive_Buff[2];				
					   break;
				
				//0x07:精准定时:1-12H
				//秒单位定时：参数一：07，参数二：需要定时的小时数，参数三：需要定时的分钟数
				case 7:Open_time=2;/*打开判断定时器工作变量为2*/	
					   User_Time=(Receive_Buff[1]*60+Receive_Buff[2]); /*计算定时总分钟数*/
					   OLED_ShowNum(9+45,2,Receive_Buff[1],2,16); //小时
					   OLED_ShowNum(9+75,2,Receive_Buff[2],2,16); //分钟
					   TIM_Cmd(TIM2, ENABLE);break;

				/*0x08:智能模式:根据温度自动调节风速、无人时候自动关闭*/
				case 8:				
				send_flag=1;//温度开
				zhuansu=1;  //转速开
				break; 
				
				/*睡眠模式*/
				case 9:
				/*1、随着温度改变PWM输出
				  2、间歇性吹风*/
				Sleep_Mode();
				break;
			}
			//printf("\r\n您发送的长度为:%d\r\n\r\n",len);
			//printf("\r\n");//插入换行
			USART_RX_STA=0;
			LED_Succeed();  /*成功接收指示灯闪烁*/
	}

}



void Keyscanf(){	
//主函数加载的函数
		/*智能模式切换按键*/ 
		if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_5)==0)
		{
			delay_ms(100);
			if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_5)==0)
			{			
				Interface+=1;
				if(Interface>2)Interface=0;
			}

		}
				/*非智能模式，传统模式*/
		if(Interface==0)
		{
			if(Interface1==0)
			{
				Interface1=1;
				printf("非智能模式\r\n");
				LED_change();   /*切换成功指示灯*/
				OLED_P6x8Str(0,0,"0");	        //智能与非智能
			}
			//每两分钟显示温度，每两分钟检测一下是否有人在
			//Check();	
		if(Read_DHT11(&DHT11_Data)==1){	
			OLED_ShowNum(55,6,DHT11_Data.humi_int,2,16);	//显示湿度	
			OLED_ShowNum(75,6,DHT11_Data.humi_deci,1,16);	//显示湿度		
			OLED_ShowNum(55,4,DHT11_Data.temp_int,2,16);	//显示温度
			OLED_ShowNum(75,4,DHT11_Data.temp_deci,1,16);	//显示温度
			
		}
		
	}
		
		
		/*智能模式*/
		if(Interface==1)
		{
			if(Interface1==1)
			{
					Interface1=2;
					OLED_P6x8Str(0,0,"1");	 //智能与非智能
					LED_change();   /*切换成功指示灯*/
					printf("智能模式\r\n");
					
			}		
			Intelligence_Mode();     //进入智能模式
		}	
		
		/*睡眠模式*/
		if(Interface==2)
		{
			if(Interface1==2)
			{
					Interface1=0;
					OLED_P6x8Str(0,0,"2");	 
					LED_change();   /*切换成功指示灯*/
					printf("睡眠模式\r\n");
					
			}	
			Sleep_Mode();     		//进入睡眠模式			
		}	
		
}
void We_are_Team(){

	/*
	1、按键控制传统转速
	2、按键控制传统定时
	3、蓝牙指令
	4、无人时候自动关闭，每1min检测一次
	5、DHT11显示，每1min读取一次
	*/
	
	//打开智能模式:0不打开 1打开 不打开的时候加载两分钟自动检测，打开的时候按智能模式来
	Keyscanf();
	
	//按键控制传统转速、传统定时
	Shift_PWM();
	
	//蓝牙指令
	Bluetooth();
	
	//两分钟检测是否有人存在，并且每1Min显示DHT11温度，已经放到智能模式的判断下了
	//Check();
	
}




int main(void){	 
//	u8 i=0;
	SystemInit();		 
	delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	
	//定时器2用来控制定制时间
	TIM2_Int_Init(7199,9999);//1s	((1+arr )/72M)*(1+psc )=((1+7199)/72M)*(1+9999)=1秒	
	TIM_Cmd(TIM2, DISABLE);  //默认关闭TIM2	
	 
	//定时器3用来控制风扇转速	
	PWM_Init(1999,0);
	TIM_Cmd(TIM3, ENABLE);  	//默认打开TIM3	
	TIM_SetCompare1(TIM3,0);    //A6，先关闭电机
	//EXTIX_Init();				/*外部中断按键*/
	
	//串口初始化放在定时器后面，不然打印数据不得
	uart_init(115200);	/*串口初始化为115200*/

	LED_Init();		  	/*LED模块初始化 C、B口都开了*/
	KEY_Init();	 		//按键端口初始化  B口
	OLED_Init();	  	/*OLED模块初始化  C口*/
	Init_face();		//界面初始化
	
	HC_SR501Init();   	/*人体红外初始化  A口*/
	
	
	//测试前等待1S
	delay_ms(1000);
	//DHT11初始化
	if(Read_DHT11(&DHT11_Data)==1){	
		OLED_ShowNum(55,6,DHT11_Data.humi_int,2,16);	//显示湿度	
		OLED_ShowNum(75,6,DHT11_Data.humi_deci,1,16);	//显示湿度		
		OLED_ShowNum(55,4,DHT11_Data.temp_int,2,16);	//显示温度
		OLED_ShowNum(75,4,DHT11_Data.temp_deci,1,16);	//显示温度
		show_GreenLED();
	}
	OLED_ShowChar(105,6,'O');	    
	OLED_ShowChar(110,6,'K');
	show_ThreeLED();
	while(1)
	{	

		if(HC_SR501_Statue()==1)show_GreenLED();
		else {
			show_RedLED();
		}
		
		We_are_Team();
		delay_ms(500);
	}	
}	



//定时器2中断服务程序
void TIM2_IRQHandler(void){
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  //清除TIMx的中断待处理位:TIM 中断源 
		time++;  /*计时时间到了之后，开始关闭风扇工作*/
		}
		if(Open_time==1)
		{
			if(time==30)        		 /*无人存在的时候关闭风扇，定时两分半：150S，因为自动检测用了两分钟*/
			{	
			time=0;
			TIM_SetCompare1(TIM3,0);  	 //A6	
		    TIM_Cmd(TIM2,DISABLE);  	 //计时结束关闭TIM2		
			LED_Close();  /*声光报警*/
			} 
		}
		
		if(Open_time==2)
		{
			if(time==(User_Time*60-1))   /*如果到了用户定时时间，那么就关机*/
			{	
			time=0;
			TIM_SetCompare1(TIM3,0);  	 //A6关闭	
			LED_Close();  /*声光报警*/
		    TIM_Cmd(TIM2,DISABLE);  	 //计时结束关闭TIM2		
			} 
		}
		
		if(Open_time==3)
		{
			if(time==(User_Time*60-1))      /*传统定时，按键没按下增加十分钟，只能定时10-180min*/
			{	
			time=0;
			TIM_SetCompare1(TIM3,0);  	  	//A6关闭	
			LED_Close();  /*声光报警*/
		    TIM_Cmd(TIM2,DISABLE);  	 	//计时结束关闭TIM2		
			} 
		}
}



////外部中断服务程序
//void EXTI9_5_IRQHandler(void)
//{
//	if(EXTI_GetITStatus(EXTI_Line5)!= RESET)  
//		{  
//		EXTI_ClearITPendingBit(EXTI_Line5);
//		delay_ms(100);//消抖
//		/*智能模式切换按键*/ 
//		Interface+=1;
//		if(Interface>1)Interface=0;
//		} 
//}

