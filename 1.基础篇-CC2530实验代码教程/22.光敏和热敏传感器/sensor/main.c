/****************************************************************************
* 文 件 名: main.c
* 描    述: 光敏实验，有光时LED1亮，用手挡住光敏电阻时LED1熄灭
****************************************************************************/
#include <ioCC2530.h>

typedef unsigned char uchar;
typedef unsigned int  uint;


#define LED1 P1_0            //定义P1.0口为LED1控制端
#define DATA_PIN P0_5        //定义P0.5口为传感器输入端

/****************************************************************************
* 名    称: InitLed()
* 功    能: 设置LED灯相应的IO口
* 入口参数: 无
* 出口参数: 无
****************************************************************************/
void InitLed(void)
{
    P1DIR |= 0x01;           //P1.0定义为输出口
}

/****************************************************************************
* 名    称: DelayMS()
* 功    能: 以毫秒为单位延时 16M时约为535,系统时钟不修改默认为16M
* 入口参数: msec 延时参数，值越大，延时越久
* 出口参数: 无
****************************************************************************/
void DelayMS(uint msec)
{ 
    uint i,j;
    
    for (i=0; i<msec; i++)
        for (j=0; j<535; j++);
}


void main(void)
{      
    P0DIR &= ~0x20;          //P0.5定义为输入口 
    InitLed();               //设置LED灯相应的IO口
    while(1)                 //死循环
    {
        if(DATA_PIN == 1)    //当光敏电阻处于黑暗中时P0.5高电平,LED1熄灭
        {
            LED1 = 1;
            DelayMS(1000);
        }
        else
        {
            LED1 =  0;       //检测到光线时P0.5为低电平LED1亮
            DelayMS(1000);
        }         
    }    
}
