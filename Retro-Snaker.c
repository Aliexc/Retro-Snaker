#include<reg51.h>
#include<intrins.h>
#include<stdlib.h>
#define uchar unsigned char
#define uint unsigned int
#define Max_snack 23
#define LCD1602_DB  P0//lcd数据端口

static uchar SnackLength;
static uchar Direction=3;
static uchar num = 0;  //中断计数变量
static uint foodx;	//食物的坐标
static uint foody;
static uint LocationX[Max_snack]={0};
static uint LocationY[Max_snack]={0};
static uint lastx;	   //用于储存前一次移动最后的一截身体
static uint lasty;
static uchar Score = 0;//得分初始化是0
uchar code scoremap[][25] ={"0000","0010","0020","0030","0040","0050","0060","0070","0080",
"0090","0100","0110","0120","0130","0140","0150","0160","0170","0180","0190","0200","0210",
"0220","0230","0240","0250"};				   //液晶显示的分数
uchar code str[] = "Your score is";

sbit up    = P2^0;   //按键端口
sbit down  = P2^3;
sbit right = P2^2;
sbit left  = P2^1;
sbit start = P2^4;

sbit LCD_RS = P2^5;	 //lcd液晶控制端口
sbit LCD_RW = P2^6;
sbit LCD_EN = P2^7;

sbit MOSIO = P3^4;  //HC595点阵控制端
sbit R_CLK = P3^5;
sbit S_CLK = P3^6;

sbit BUZZER = P3^7;
sbit BEGAIN = P3^0;

/*
****************************************
*      延时函数
****************************************
*/

void delay(uint time)
{	uint i;
	for(time;time>0;time--)
	for(i=0;i<90;i++);
}

 /*
****************************************
*      这里是LCD液晶有关的函数
****************************************
*/

void ready_busy()//检测液晶是否忙
{
	 uchar  sta;
	 LCD1602_DB = 0xff;
	 LCD_RS = 0;
	 LCD_RW = 1;
	 do
     {
         LCD_EN = 1;
         sta = LCD1602_DB;
         LCD_EN = 0;

     }while(sta & 0x80);
}

void LCD_cmd(uchar cmd)	//液晶命令传输
{
	ready_busy();
	LCD_RS = 0;
	LCD_RW = 0;
	LCD1602_DB = cmd;
	LCD_EN = 1;
	LCD_EN = 0;
}

void LCD_dat(uchar dat)//液晶数据传输
{
    ready_busy();
    LCD_RS = 1;
    LCD_RW = 0;
    LCD1602_DB = dat;
    LCD_EN = 1;
    LCD_EN = 0;
}

void LCD_show(uchar x,uchar y,uchar *str)//液晶显示程序
{
    uchar addr;
    if(y==0)
        addr = 0x00+x;
    else
        addr = 0x40+x;
    LCD_cmd(addr|0x80);
    while (*str != '\0')
    {
        LCD_dat(*str++);
    }
}

/*
****************************************
*      这里是HC595写入数据的函数
****************************************
*/
void HC595SendData(uchar BT3,uchar BT2,uchar BT1,uchar BT0)
{
	uchar i;
	for(i=0;i<8;i++)
	{
		MOSIO = BT3 >> 7;
		BT3 <<= 1;

		S_CLK = 0;
		S_CLK = 1;
	}
	for(i=0;i<8;i++)
	{
		MOSIO = BT2 >>7;
		BT2 <<= 1;

		S_CLK = 0;
		S_CLK = 1;
	}
	for(i=0;i<8;i++)
	{
		MOSIO = BT1 >> 7;
		BT1 <<= 1;
		S_CLK = 0;
		S_CLK = 1;
	}
	for(i=0;i<8;i++)
	{
		MOSIO = BT0 >> 7;
		BT0 <<= 1;
		S_CLK = 0;
		S_CLK = 1;
	}

	R_CLK = 0;
	R_CLK = 1;
	R_CLK = 0; //通过上升沿把数据写到点阵上
}

/*
****************************************
*      这里是蛇身显示关的函数
****************************************
*/

void Change(uint *Location)//穿墙模式
{
	if((*Location)==0)		    
	*Location=16;

	if((*Location)==17)
	*Location=1;
}

uint xChange(uint *x)//将第x列转换成16进制
{
	Change(x);
	switch(*x)
	{
		case 1:return 0x8000;
		case 2:return 0x4000;
		case 3:return 0x2000;
		case 4:return 0x1000;
		case 5:return 0x0800;
		case 6:return 0x0400;
		case 7:return 0x0200;
		case 8:return 0x0100;
		case 9:return 0x0080;
		case 10:return 0x0040;
		case 11:return 0x0020;
		case 12:return 0x0010;
		case 13:return 0x0008;
		case 14:return 0x0004;
		case 15:return 0x0002;
		case 16:return 0x0001;
		default:return 0;
	}
}

uint yChange(uint *y)//将第y列转换成16进制
{
    Change(y);
	switch(*y)
	{
		case 1:return 0x7fff;
		case 2:return 0xbfff;
		case 3:return 0xdfff;
		case 4:return 0xefff;
		case 5:return 0xf7ff;
		case 6:return 0xfbff;
		case 7:return 0xfdff;
		case 8:return 0xfeff;
		case 9:return 0xff7f;
		case 10:return 0xffbf;
		case 11:return 0xffdf;
		case 12:return 0xffef;
		case 13:return 0xfff7;
		case 14:return 0xfffb;
		case 15:return 0xfffd;
		case 16:return 0xfffe;
		default:return 0;
	}
}


void display(uint *x,uint *y)//在坐标先X,Y点显示
{
	uchar i,H1,H2,L1,L2;
	for(i=0;i<SnackLength;i++)
	{
		H1 = xChange(x+i)/256;
		H2 = xChange(x+i)%256;
		L1 = yChange(y+i)/256;
		L2 = yChange(y+i)%256;
		HC595SendData(H1,H2,L1,L2);
		HC595SendData(xChange(&foodx)/256,xChange(&foodx)%256,yChange(&foody)/256,yChange(&foody)%256);
    }
}

void Createfood()
{
    uchar i;
	recreate:
     foodx = (TL1%16)+1;//因为计数器里的数据实时更新所以用来获取食物坐标
     foody = (TL1%16)+1;
	 for(i = 0;i<SnackLength-1;i++)
    {
        if((LocationX[i]==foodx)&&(LocationY[i]==foody))
            goto recreate;
    }
}

void init()//初始化蛇身
{   
	uchar i;
 	LCD_cmd(0x38);	 //初始化液晶
    LCD_cmd(0x0c);
    LCD_cmd(0x06);
    LCD_cmd(0x01);
	Createfood();  //创造食物
	SnackLength = 2;
	Direction=3;  //初始化方向
	Score=0;  //初始化分数
	for(i=0;i<Max_snack;i++)	//初始化蛇身数组
	{
		LocationX[i]=100;
		LocationY[i]=100;
	}

		LocationX[0]=2;
		LocationY[0]=1;

		LocationX[1]=1;
		LocationY[1]=1;
}

void Is_eat()//判断是否迟到食物函数
{

    if(foodx==LocationX[0]&&foody==LocationY[0])
    {
        SnackLength++;
		Score++;
        Createfood();
        LocationX[SnackLength-1] = lastx;
        LocationY[SnackLength-1] = lasty;
	}
}

void Is_touch()
{
    uchar i;
    for(i =1;i<SnackLength;i++)
    {
        if((LocationX[0]==LocationX[i])&&(LocationY[0]==LocationY[i]))
        {
			BUZZER=0;
            delay(80);
			BUZZER=1;
            init();
        }
    }
}


void keyScan(uchar dir)//扫描键盘并且刷新方向值
{

    if(up==0&&dir!=3)
    dir=2;

    else if(down==0&&dir!=2)
    dir=3;

    else if(left==0&&dir!=1)
    dir=0;

    else if(right==0&&dir!=0)
    dir=1;

    else
    dir = dir;
	Direction = dir;
}



void snakemove()   //蛇身移动程序
{
   uchar i;
   lastx = LocationX[SnackLength-1]; //保留每一次移动前最后一截
   lasty = LocationY[SnackLength-1]; //用来给增长的蛇身
   switch(Direction)
   {
    case 0:
        for(i=SnackLength-1;i>0;i--)
		{
			LocationX[i]=LocationX[i-1];
			LocationY[i]=LocationY[i-1];
		}
		LocationY[0]--;
        break;
    case 1:
        for(i=SnackLength-1;i>0;i--)
		{
			LocationX[i]=LocationX[i-1];
			LocationY[i]=LocationY[i-1];
		}
		LocationY[0]++;
        break;
    case 2:
        for(i=SnackLength-1;i>0;i--)
		{
			LocationX[i]=LocationX[i-1];
			LocationY[i]=LocationY[i-1];
		}
		LocationX[0]--;
        break;
    case 3:
        for(i=SnackLength-1;i>0;i--)
		{
			LocationX[i]=LocationX[i-1];
			LocationY[i]=LocationY[i-1];
		}
		LocationX[0]++;
        break;

   }

}

void time_0() interrupt 1	 //中断函数
{
    TH0 = (65536-50000)/256;
	TL0 = (65536-50000)%256;
    keyScan(Direction);//每50ms进行一次键盘扫
	num++;
}

void time_1() interrupt 3
{
	TH1 = 0;
	TL1 = 0;
}

void main()
{
	uchar spend;
	EA = 1;//中断总使能
	ET0 = 1;//T0打开
	ET1 = 1;
	TMOD = 0X11;//中断功能选项
	TR0 = 1;//启动计时
	TR1 = 1;
	TH0 = (65536-50000)/256;//初值装载
	TL0 = (65536-50000)%256;
	TH1 = 0;
	TL1 = 0;

   init();
   while(BEGAIN==0)
   while(1)
	{
	  display(LocationX,LocationY);
	  	if(num==spend) //移动速度
	  	{
	  		num = 0;
			snakemove();
			LCD_show(0,0,str);
			LCD_show(6,1,scoremap[Score]);
			Is_eat(); //检测是否吃到
			Is_touch();
			if(BEGAIN==0)
			init();

			if(SnackLength<=5)
			spend=15;
			else if(SnackLength<=10)
			spend=10;
			else
			spend=5;
	 	}
	 }	 
}
