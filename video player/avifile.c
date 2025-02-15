#include "avifile.h"
#include <stdio.h>
#include <string.h>
#include "led.h"
#include "lcd.h"
#include "usart.h"

/****************************************************************************
	�������ӹ�����
	GD STM32F407ѧϰ��
	�Ա��꣺http://shop71381140.taobao.com/
	������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
****************************************************************************/

AVI_TypeDef AVI_file;
avih_TypeDef* avihChunk;
strh_TypeDef* strhChunk;
BITMAPINFO* bmpinfo;
WAVEFORMAT* wavinfo;
u32 temp=0x00;
u8 vids_ID;
u8 auds_ID;

u8 AVI_Parser(u8 *buffer)
{
	temp=ReadUnit(buffer,0,4,1);//��"RIFF"
	if(temp!=RIFF_ID)return 1;
	AVI_file.RIFFchunksize=ReadUnit(buffer,4,4,1);//RIFF���ݿ鳤��
	temp=ReadUnit(buffer,8,4,1);//��"AVI "
	if(temp!=AVI_ID)return 2;
	temp=ReadUnit(buffer,12,4,1);//��"LIST"
	if(temp!=LIST_ID)return 3;
	AVI_file.LISTchunksize=ReadUnit(buffer,16,4,1);//LIST���ݿ鳤��
	temp=ReadUnit(buffer,20,4,1);//��"hdrl"
	if(temp!=hdrl_ID)return 4;
	temp=ReadUnit(buffer,24,4,1);//��"avih"
	if(temp!=avih_ID)return 5;
	AVI_file.avihsize=ReadUnit(buffer,28,4,1);//avih���ݿ鳤��	
	return 0;				
}

u8 Avih_Parser(u8 *buffer)
{
	avihChunk=(avih_TypeDef*)buffer;
#ifdef PrintInfo
	printf("\r\navih���ݿ���Ϣ:\n");
	printf("\r\nSecPerFrame:%d",avihChunk->SecPerFrame);
	printf("\r\nMaxByteSec:%d",avihChunk->MaxByteSec);
	printf("\r\nChunkBase:%d",avihChunk->ChunkBase);
	printf("\r\nSpecProp:%d",avihChunk->SpecProp);
	printf("\r\nTotalFrame:%d",avihChunk->TotalFrame);
	printf("\r\nInitFrames:%d",avihChunk->InitFrames);
	printf("\r\nStreams:%d",avihChunk->Streams);
	printf("\r\nRefBufSize:%d",avihChunk->RefBufSize);
	printf("\r\nWidth:%d",avihChunk->Width);
	printf("\r\nHeight:%d",avihChunk->Height);
#endif
	if((avihChunk->Width>320)||(avihChunk->Height>240))return 1;//��Ƶ�ߴ粻֧��
	if(avihChunk->Streams!=2)return 2;//��Ƶ������֧��
	return 0;
}

u8 Strl_Parser(u8 *buffer)
{
	temp=ReadUnit(buffer,0,4,1);//��"LIST"
	if(temp!=LIST_ID)return 1;
	AVI_file.strlsize=ReadUnit(buffer,4,4,1);//strl���ݿ鳤��
	temp=ReadUnit(buffer,8,4,1);//��"strl"
	if(temp!=strl_ID)return 2;
	temp=ReadUnit(buffer,12,4,1);//��"strh"
	if(temp!=strh_ID)return 3;
	AVI_file.strhsize=ReadUnit(buffer,16,4,1);//strh���ݿ鳤��
	strhChunk=(strh_TypeDef*)(buffer+20);		 //108
#ifdef PrintInfo
	printf("\r\nstrh���ݿ���Ϣ:\n");	
	printf("\r\nStreamType:%s",strhChunk->StreamType);
	printf("\r\nHandler:%s",strhChunk->Handler);//��������MJPEG
	printf("\r\nStreamFlag:%d",strhChunk->StreamFlag);
	printf("\r\nPriority:%d",strhChunk->Priority);
	printf("\r\nLanguage:%d",strhChunk->Language);
	printf("\r\nInitFrames:%d",strhChunk->InitFrames);
	printf("\r\nScale:%d",strhChunk->Scale);
	printf("\r\nRate:%d",strhChunk->Rate);
	printf("\r\nStart:%d",strhChunk->Start);
	printf("\r\nLength:%d",strhChunk->Length);
	printf("\r\nRefBufSize:%d",strhChunk->RefBufSize);
	printf("\r\nQuality:%d",strhChunk->Quality);
	printf("\r\nSampleSize:%d",strhChunk->SampleSize);
	printf("\r\nFrameLeft:%d",strhChunk->Frame.Left);
	printf("\r\nFrameTop:%d",strhChunk->Frame.Top);
	printf("\r\nFrameRight:%d",strhChunk->Frame.Right);
	printf("\r\nFrameBottom:%d",strhChunk->Frame.Bottom);
#endif
	if(strhChunk->Handler[0]!='M')return 4;
	return 0;
}

u8 Strf_Parser(u8 *buffer)
{
	temp=ReadUnit(buffer,0,4,1);//��"strf"
	if(temp!=strf_ID)return 1;
	if(strhChunk->StreamType[0]=='v')//��һ����Ϊ��Ƶ��
	{
		vids_ID='0';
		auds_ID='1';
		bmpinfo=(BITMAPINFO*)(buffer+8);
		wavinfo=(WAVEFORMAT*)(buffer+4332);
	}
	else if(strhChunk->StreamType[0]=='a')//��һ����Ϊ��Ƶ��
	{
		vids_ID='1';
		auds_ID='0';
		wavinfo=(WAVEFORMAT*)(buffer+8);
		bmpinfo=(BITMAPINFO*)(buffer+4332);
	}
#ifdef PrintInfo		
	printf("\r\nstrf���ݿ���Ϣ(��Ƶ��):\n");		
	printf("\r\n���ṹ���С:%d",bmpinfo->bmiHeader.Size);
	printf("\r\nͼ���:%d",bmpinfo->bmiHeader.Width);
	printf("\r\nͼ���:%d",bmpinfo->bmiHeader.Height);
	printf("\r\nƽ����:%d",bmpinfo->bmiHeader.Planes);
	printf("\r\n����λ��:%d",bmpinfo->bmiHeader.BitCount);
	printf("\r\nѹ������:%s",bmpinfo->bmiHeader.Compression);
	printf("\r\nͼ���С:%d",bmpinfo->bmiHeader.SizeImage);
	printf("\r\nˮƽ�ֱ���:%d",bmpinfo->bmiHeader.XpixPerMeter);
	printf("\r\n��ֱ�ֱ���:%d",bmpinfo->bmiHeader.YpixPerMeter);
	printf("\r\nʹ�õ�ɫ����ɫ��:%d",bmpinfo->bmiHeader.ClrUsed);
	printf("\r\n��Ҫ��ɫ:%d",bmpinfo->bmiHeader.ClrImportant);

	printf("\r\nstrf���ݿ���Ϣ(��Ƶ��):\n");
	printf("\r\n��ʽ��־:%d",wavinfo->FormatTag);
	printf("\r\n������:%d",wavinfo->Channels);
	printf("\r\n������:%d",wavinfo->SampleRate);
	printf("\r\n������:%d",wavinfo->BaudRate);
	printf("\r\n�����:%d",wavinfo->BlockAlign);
	printf("\r\n���ṹ���С:%d",wavinfo->Size);
#endif
	return 0;
}

u16 Search_Movi(u8* buffer)
{
	u16 i;
	for(i=0;i<20480;i++)
	{
	   	if(buffer[i]==0x6d)
			if(buffer[i+1]==0x6f)
				if(buffer[i+2]==0x76)	
					if(buffer[i+3]==0x69)return i;//�ҵ�"movi"	
	}
	return 0;		
}

u16 Search_Fram(u8* buffer)
{
	u16 i;
	for(i=0;i<20480;i++)
	{
	   	if(buffer[i]=='0')
			if(buffer[i+1]==vids_ID)
				if(buffer[i+2]=='d')	
					if(buffer[i+3]=='c')return i;//�ҵ�"xxdc"	
	}
	return 0;		
}

u32 ReadUnit(u8 *buffer,u8 index,u8 Bytes,u8 Format)//1:���ģʽ;0:С��ģʽ
{
  	u8 off=0;
  	u32 unit=0;  
  	for(off=0;off<Bytes;off++)unit|=buffer[off+index]<<(off*8);
  	if(Format)unit=__REV(unit);//���ģʽ
  	return unit;
}


