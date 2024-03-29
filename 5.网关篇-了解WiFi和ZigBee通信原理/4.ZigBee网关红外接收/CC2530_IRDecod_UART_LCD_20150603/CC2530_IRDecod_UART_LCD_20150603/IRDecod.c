/***************************************************************************
* 文 件 名: IRRecod.c
* 作    者: 创思通信
* 修    订: 2015-06-08
* 版    本: V1.0
* 描    述: 红外解码
****************************************************************************
红外结构（经过红外接收头后的电平变量及解码方式）：

			|    引导码   |	 用户码第0位最低位 |	用户码第1位		|一直到第15位，然后是操作码，和用户码一样也是15位
逻辑表示	|			  |  	逻辑0	 	   |		逻辑1		|

电平高低	|  低   高	  |	低		高		   |	低		高		|

电平持续时间| 9ms  4.5ms  |	0.56ms  0.56ms	   |	0.56ms	1.685ms	|

说明：通过测量电平持续时间来解码出0和1来，9ms和4.5ms用来识别解码开始，然后
	  以高电平0.56ms、低电平0.56ms、周期为1.125ms的组合表示二进制的“0”；
	  以高电平0.56ms、低电平1.685ms、周期为2.25ms的组合表示二进制的“1”
	  一直解码32位，就组成了4字节的红外码值



举例：

位数说明 		：从用户码第15位开始到0位	从操作码第15位开始到0位

二进制逻辑表示	：0000 0000  1111 1111		0100 0101 	1011 1010

十六进制表示	：0x00 		 0xff			0x45		0xBA

字节说明		：用户码     用户反码       操作码      操作码反码 

说明			：用户码同一遥控器基本一样，不同按钮只有操作码不同
				  反码是NEC协议提高通讯可靠的一种手段，用于验证解码正确性，
				  就是二进制把1变0 0变1  0x00反码是0xFF  0x45同理，反码是0xBA，
			  	  大分部电视、机顶盒、DVD等都是NEC协议，大部分空调和其它设备都
				  是自定协议,比NEC复杂的多

*******************************************************************************/

/*******************************包含头文件*************************************/
#include "IRDecod.h"		//红外解码头文件
#include "uart2.h"			//串口头文件
#include "lcd12864.h"		//LCD显示屏头文件
/********************************本地变量**************************************/
uint16 HWAddcode,HWComcode;	//红外地址码 操作码
uint16 HWdat[66];			//红外电平长度缓存
/*******************************************************************************
* 函数名称：IRDelay_10us
* 说    明：红外解码用延时函数 10微秒
* 引用说明：无
* 返    回：无
*******************************************************************************/
void IRDelay_10us(void)
{                         
    unsigned int a=7;
    while(a)
    {
        a--;
    }
}
/*******************************************************************************
* 函数名称：IRInit
* 说    明：IR初始化函数 初始化红外接收头端口
* 引用说明：无
* 返    回：无
*******************************************************************************/
void IRInit(void)
{
	P0SEL &= ~0x20;		//P0_5 设置为普通IO口  
	P0DIR &= ~0x20;		//P0_5 设置为输入模式 	
	P0INP &= ~0x20;		//打开P0_5上拉电阻,不影响
}
/*******************************************************************************
* 函数名称：IR_Decoding
* 说    明：IR_Decoding红外解码函数
* 引用说明：先调用IRInit初始化
* 返    回：0 解码正确
			1 引导码错误
			2 用户识别码(地址码)错误
			3 操作码（指令码）错误
			4 超时出错
*******************************************************************************/
uint8 IR_Decoding(void)
{
	uint16 LLevel = 0;		//低电平时长变量
	uint16 HLevel = 0;		//高电平时长变量
	uint8 i=0;				//循环变量
	uint8 AntiCode1 = 0;	//反码验证变量
	uint8 AntiCode2 = 0;	//反码验证变量	
	HWAddcode = 0;			//红外地址码变量归零
	HWComcode = 0;			//红外指令码变量归零
	for(i=0;i<33;i++)		//引导码2bit + 16bit用户识别码 + 16bit操作码
	{
		HLevel = 0;			//高电平时长归零
		LLevel = 0;			//低电平时长归零
		while(IR_IRQ==0)	//低电平
		{
			LLevel++;		//低电平++
			IRDelay_10us();	//延时10us
			if(HLevel>1100)return 4;	//超时出错
		}
                
		while(IR_IRQ==1)	//高电平
		{
			HLevel++;		//高电平++
			IRDelay_10us();	//延时10us
			if(HLevel>1100)return 4;	//超时出错
		}
		HWdat[i*2] = LLevel;	//保存低电平时长
		HWdat[(i*2)+1] = HLevel;//保存高电平时长
	}
	if((HWdat[0]<1100)&&(HWdat[0]>800)&&(HWdat[1]<550)&&(HWdat[1]>350))//引导码识别 9ms + 4.5ms
	{
		for(i=0;i<16;i++)//16bit用户识别码(地址码)识别
		{
			if((HWdat[i*2+2]<70)&&(HWdat[i*2+2]>40)&&(HWdat[(i*2)+1+2]<70)&&(HWdat[(i*2)+1+2]>40))//识别逻辑0
			{
			}
			else if((HWdat[i*2+2]<70)&&(HWdat[i*2+2]>40)&&(HWdat[(i*2)+1+2]<195)&&(HWdat[(i*2)+1+2]>140))//识别逻辑1
			{
				HWAddcode = HWAddcode | (1<<i);//逻辑1 组成两个字节
			}
			else return 2;//用户识别码(地址码)错误
		}
		AntiCode1 = (HWAddcode>>8)&0xff;//取出高八位
		if(~AntiCode1!=HWAddcode&0xff)//反码验证
		{
			return 2;//用户识别码(地址码)反码验证错误
		}
		for(i=0;i<16;i++)//16bit操作码（指令码）识别
		{
			if((HWdat[i*2+34]<70)&&(HWdat[i*2+34]>40)&&(HWdat[(i*2)+1+34]<70)&&(HWdat[(i*2)+1+34]>40))//识别逻辑0
			{
			}
			else if((HWdat[i*2+34]<70)&&(HWdat[i*2+34]>40)&&(HWdat[(i*2)+1+34]<195)&&(HWdat[(i*2)+1+34]>140))//识别逻辑1
			{
				HWComcode = HWComcode | (1<<i);//逻辑1 组成两个字节
			}
			else return 3;//操作码（指令码）错误
		}
		AntiCode2 = (HWComcode>>8)&0xff;//取出高八位
		AntiCode2=~AntiCode2;
		if(AntiCode2!=(uint8)(HWComcode&0x00ff))//反码验证
		{
			return 5;//用操作码（指令码）反码验证错误
		}		
	}else return 1;//引导码错误
	return 0;//解码正确
}
