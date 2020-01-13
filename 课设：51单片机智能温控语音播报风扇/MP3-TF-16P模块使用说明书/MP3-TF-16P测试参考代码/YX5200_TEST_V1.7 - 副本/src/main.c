/******************************************************************************************************************
        
							+--------------------------------------------------+
							             	 主函数部分
							+--------------------------------------------------+

 - 实现功能：

 - 目前进展：
 - 日期    ：2013-05-06

 - 作者    ：

 - 运行环境：STC   晶振：11.0592M     波特率:9600

 - 备注    ：在普中科技的51开发板上调试OK --- STC89C516RD+
 
 1、实现芯片上电分别指定播放第一曲和第二曲，基本的程序供用户测试
 2、该测试程序必须是模块或者芯片方案中，有设备在线，譬如U盘、TF卡、FLASH等等
 3、
******************************************************************************************************************/
#include "reg52.h"


#define TIMER0_H   (65536-1800)/256//定时2Ms
#define TIMER0_L   (65536-1800)%256

#define SYS_Fosc        12000000  //晶振频率			   
#define COMM_BAUD_RATE  9600      //串口波特率

#define OSC_FREQ        12000000  //11059200  

#define u8 unsigned char
#define u16 unsigned int
static u8 Send_buf[10] = {0} ;
static u8 Recv_buf[10] = {0} ;

static u8 SendDataLen = 0 ;
static u8 ResendDataLen = 0 ;

/******************************串口1的波特率********************************/
//T1作波特率发生器
//在波特率加倍情况下 
#define BAUD_57600    			256 - (OSC_FREQ/192L)/57600L    // 254 FF
#define BAUD_28800    			256 - (OSC_FREQ/192L)/28800L    // 254 FE
#define BAUD_19200    			256 - (OSC_FREQ/192L)/19200L    // 253 FD
#define BAUD_14400    			256 - (OSC_FREQ/192L)/14400L    // 252 FC
#define BAUD_9600     			256 - (OSC_FREQ/192L)/9600L     // 250 FA

/*****************************************************************************************************
 - 功能描述：10us的延时函数
 - 隶属模块：常用函数库(内部)
 - 参数说明：无
 - 返回参数：无
 - 注： 在这里的运行环境是51,晶振为12MHZ		
*****************************************************************************************************/
void Delay_Us(u8 z)
{
	while(z--);
}

/***********************毫秒级别延时************************/

void Delay_Ms(u16 z)
{
	u8 x=0 , y=0;
	for(x=110 ; x>0 ;x--)
	for(y=z; y>0;y-- );
}

/*****************************************************************************************************
 - 功能描述： 串口1初始化
 - 隶属模块： 外部
 - 参数说明： 无
 - 返回说明： 无
 - 注：	      都是9600波特率
*****************************************************************************************************/
void Serial_init(void)
{
	TMOD = 0x20;                // 设置 T1 为波特率发生器
	SCON = 0x50;                // 0101,0000 8位数据位, 无奇偶校验
				   		
	PCON = 0x00;                //PCON=0;

	TH1=256-(SYS_Fosc/COMM_BAUD_RATE/32/12);//设置为9600波特率
	TL1=256-(SYS_Fosc/COMM_BAUD_RATE/32/12);

    TR1     = 1; 			   //定时器1打开
    REN     = 1;			   //串口1接收使能
    ES      = 1;			   //串口1中断使能
}


/********************************************************************************************
 - 功能描述： 串口发送一个字节
 - 隶属模块： 外部
 - 参数说明：
 - 返回说明：
 - 注：	      
********************************************************************************************/
void Uart_PutByte(u8 ch)
{
    SBUF  = ch;
    while(!TI){;}
    TI = 0;
}

/*****************************************************************************************************
 - 功能描述： 串口发送一帧数据
 - 隶属模块： 内部 
 - 参数说明： 
 - 返回说明： 
 - 注：无     
*****************************************************************************************************/
void SendCmd(u8 len)
{
    u8 i = 0 ;

    Uart_PutByte(0x7E); //起始
    for(i=0; i<len; i++)//数据
    {
		Uart_PutByte(Send_buf[i]) ;
    }
    Uart_PutByte(0xEF) ;//结束
}

/********************************************************************************************
 - 功能描述：求和校验
 - 隶属模块：
 - 参数说明：
 - 返回说明：
 - 注：      和校验的思路如下
             发送的指令，去掉起始和结束。将中间的6个字节进行累加，最后取反码
             接收端就将接收到的一帧数据，去掉起始和结束。将中间的数据累加，再加上接收到的校验
             字节。刚好为0.这样就代表接收到的数据完全正确。
********************************************************************************************/
void DoSum( u8 *Str, u8 len)
{
    u16 xorsum = 0;
    u8 i;

    for(i=0; i<len; i++)
    {
        xorsum  = xorsum + Str[i];
    }
	xorsum     = 0 -xorsum;
	*(Str+i)   = (u8)(xorsum >>8);
	*(Str+i+1) = (u8)(xorsum & 0x00ff);
}


/********************************************************************************************
 - 功能描述： 串口向外发送命令[包括控制和查询]
 - 隶属模块： 外部
 - 参数说明： CMD:表示控制指令，请查阅指令表，还包括查询的相关指令
              feedback:是否需要应答[0:不需要应答，1:需要应答]
              data:传送的参数
 - 返回说明：
 - 注：       
********************************************************************************************/
void Uart_SendCMD(u8 CMD ,u8 feedback , u16 dat)
{
    Send_buf[0] = 0xff;    //保留字节 
    Send_buf[1] = 0x06;    //长度
    Send_buf[2] = CMD;     //控制指令
    Send_buf[3] = feedback;//是否需要反馈
    Send_buf[4] = (u8)(dat >> 8);//datah
    Send_buf[5] = (u8)(dat);     //datal
    DoSum(&Send_buf[0],6);        //校验
    SendCmd(8);       //发送此帧数据
}

void main()
{ 
	Delay_Us(1) ;
	Delay_Ms(1) ;

	Serial_init() ;

    Delay_Ms(2000) ;//延时大概6S
    
    Uart_SendCMD(0x03 , 0 , 0x02) ;//播放第一首
    
    Delay_Ms(2000) ;//延时大概6S
    
    Uart_SendCMD(0x03 , 0 , 0x01) ;//播放第二首

    Delay_Ms(2000) ;//延时大概6S

    Uart_SendCMD(0x03 , 0 , 0x06) ;//播放第四首

    while(1) ;
}