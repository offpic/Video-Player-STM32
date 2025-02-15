#include "vidoplayer.h"
#include "avifile.h"
#include "usart.h" 
#include <stdio.h>
#include <string.h>
#include "delay.h"
#include "timer.h"
#include "lcd.h"
#include "led.h"
#include "key.h"
#include "touch.h"
#include "cs4334.h"
#include "sdio_sd.h"
#include "ff.h"
#include "tjpgd.h"
#include "pic.h"

/****************************************************************************
	�������ӹ�����
	GD STM32F407ѧϰ��
	�Ա��꣺http://shop71381140.taobao.com/
	������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
****************************************************************************/

FATFS fs;
DIR vdir;                          
FIL fileR;
FILINFO finfo;
UINT BytesRD;
u8  Frame_buf[20480]={0x00};
u8  Sound_buf1[8*1024]={0x00};
u8  Sound_buf2[8*1024]={0x00};
u8* Dbuf=Frame_buf;
JDEC jd;
u32  mid;
u32  Strsize;
u16  Strtype;
u32  Audsize;
__IO u8 XferCplt=1;
__IO u8 audstop=0;
u8   buffer_switch=1;
u8   progress=0;
u8   finsh=0;
TCHAR  path[30];

extern void  DispImage(char *file,u16 StartX,u16 StartY,u16 width,u16 height);
void AVI_play(void)
{
	u8  res;

 	res=f_mount(0,&fs);
	res=f_opendir(&vdir,"0:/AVI");
	while(1)
	{
		LCD_Clear(BLACK);
		DispImage("0:/pic/BT2.bmp",0,230,240,95);			
		strcpy(path,"0:/AVI/");
		LCD_Mode(4);
		LCD_Window(239,319,240,320);		
		res=f_readdir(&vdir,&finfo);
		strcat(path,finfo.fname);
		//LCD_String(20,80,path,RED);
		res=f_open(&fileR,path,FA_READ);
		while(res)LCD_String(20,50,"Cannot open file!",RED);			
		res=f_read(&fileR,Dbuf,20480,&BytesRD);		
		res=AVI_Parser(Dbuf);//����AVI�ļ���ʽ
		while(res)LCD_String(20,50,"File format error!",RED);
		res=Avih_Parser(Dbuf+32);//����avih���ݿ�
		while(res)LCD_String(20,50,"File not supported!",RED);
		res=Strl_Parser(Dbuf+88);//����strh���ݿ�
		while(res)LCD_String(20,50,"File format error!",RED);
		res=Strf_Parser(Dbuf+164);//����strf���ݿ�
		while(res)LCD_String(20,50,"File format error!",RED);		
		mid=Search_Movi(Dbuf);//Ѱ��movi ID		
		while(!mid)LCD_String(20,50,"File format error!",RED);
		Strtype=MAKEWORD(Dbuf+mid+6);//������
		Strsize=MAKEDWORD(Dbuf+mid+8);//����С
		if(Strsize%2)Strsize++;//������1		
		f_lseek(&fileR,mid+12);//������־ID		
		jd_init(&jd);//JPG�����ʼ��
		AUDIO_Init(I2S_AudioFreq_16k);
		Audio_MAL_Play((u32)Sound_buf1,4*1024);
		finsh=0;
		audstop=0;
		while(!finsh)//����ѭ��
		{					
			if(Strtype==T_vids)//��ʾ֡
			{
				Dbuf=Frame_buf;
				f_read(&fileR,Dbuf,Strsize+8,&BytesRD);//������֡+��һ������ID��Ϣ
				res=jd_prepare(&jd,Dbuf);	
				if(!res)res=jd_decomp(&jd);
			}//��ʾ֡
			else if(Strtype==T_auds)
			{				
				while(!XferCplt);//�ȴ�DMA��ʼ����
				XferCplt=0;
				if(!buffer_switch)
				{				
					f_read(&fileR,Sound_buf2,Strsize+8,&BytesRD);//���buffer2		   		
					Dbuf=Sound_buf2;
			    }
			 	else 
			   	{   				
					f_read(&fileR,Sound_buf1,Strsize+8,&BytesRD);//���buffer1
					Dbuf=Sound_buf1;
			    } 
				Audsize=Strsize;
								
			}
			else break;
			Strtype=MAKEWORD(Dbuf+Strsize+2);//������
			Strsize=MAKEDWORD(Dbuf+Strsize+4);//����С									
			if(Strsize%2)Strsize++;//������1
			Prog_ctl();									   	
		}
		f_close(&fileR);
		delay_ms(500);
	}
}

void AUDIO_TransferComplete(u32 pBuffer, u32 Size)
{  
	if(audstop)return;
	if(buffer_switch)
	{
		Audio_MAL_Play((u32)Sound_buf1,Audsize);//��buffer1����
		buffer_switch=0;
	}
	else 
	{
		Audio_MAL_Play((u32)Sound_buf2,Audsize);//��buffer2����
		buffer_switch=1;
	}
	XferCplt=1;//DMA��ʼ����
}

void Draw_pic(u16 xpos,u16 ypos,u16 width,u16 height,u8* pic)
{
  	u16 i,bytes;

	bytes=width*height*2;
	LCD_Window(xpos+width,ypos+height,width,height);
  	LCD_Cursor(xpos, ypos); 
  	LCD_REG=0x22;//WriteRAM_Prepare 
	for(i=0;i<bytes;i+=2)LCD_RAM=(pic[i]<<8)|pic[i+1];
}

void Clear_Rec(u16 xpos,u16 ypos,u16 width,u16 height,u16 color)
{          
	u16 i,bytes;

	bytes=width*height;
	LCD_Window(xpos+width,ypos+height,width,height);
  	LCD_Cursor(xpos, ypos); 
  	LCD_REG=0x22;
	for(i=0;i<bytes;i++)LCD_RAM=color;					  	    
}

void Prog_ctl(void)
{
	u16 xpos,ypos;
	u32	cursor=0;

	Clear_Rec(progress-15,200,30,29,BLACK);
	progress=f_tell(&fileR)/(f_size(&fileR)/200)+20;	
	if(PEN_int==RESET)//��������
	{
		xpos=Read_XY(CMD_RDX);
		ypos=Read_XY(CMD_RDY);
		printf("\r\nY����:%d",ypos);
		if(ypos>1200&&ypos<1370)//����������
		{
			audstop=1;
			Dbuf=Frame_buf;
			while(PEN_int==RESET)
			{			 	
				xpos=Read_XY(CMD_RDX);//260~1780
				if(xpos<260)xpos=260;
				if(xpos>1780)xpos=1780;
				Clear_Rec(progress-15,200,30,29,BLACK); 
				progress=(xpos*20)/152-14;
				Draw_pic(progress-15,200,30,29,(u8*)Image_pro2);				
				cursor=(progress-20)*(f_size(&fileR)/200)+1024;
				if(cursor>f_size(&fileR))cursor=f_size(&fileR);
				f_lseek(&fileR,cursor);			
				f_read(&fileR,Dbuf,20480,&BytesRD);
				mid=Search_Fram(Dbuf);
				Strtype=MAKEWORD(Dbuf+mid+2);//������
				Strsize=MAKEDWORD(Dbuf+mid+4);//����С									
				if(Strsize%2)Strsize++;//������1
				f_lseek(&fileR,cursor+mid+8);				
				f_read(&fileR,Dbuf,Strsize,&BytesRD);//������֡
				jd_prepare(&jd,Dbuf);	
				jd_decomp(&jd);				
			}
			f_lseek(&fileR,cursor+mid+8);
			LCD_Circle(progress,215,12,BLACK);
			Audio_MAL_Play((u32)Sound_buf1,4*1024);
			audstop=0;
		}
		else if(ypos>1370&&xpos>720&&xpos<1320)//��ͣ����
		{
			audstop=1;
			DispImage("0:/pic/BT1.bmp",0,230,240,95);
			while(PEN_int==RESET);
			while(1)
			{
				xpos=Read_XY(CMD_RDX);
				ypos=Read_XY(CMD_RDY);
				if(ypos>1370&&xpos>720&&xpos<1320)break;	
			}
			while(PEN_int==RESET);
			DispImage("0:/pic/BT2.bmp",0,230,240,95);
			Audio_MAL_Play((u32)Sound_buf1,4*1024);
			audstop=0;
		}
		else if(ypos>1370&&xpos>1320)//��һ�ļ�
		{			
			audstop=1;
			finsh=1;
		}
	}
	Draw_pic(progress-15,200,30,29,(u8*)Image_pro1);
}

