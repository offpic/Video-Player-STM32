#include "main.h"

/****************************************************************************
	�������ӹ�����
	GD STM32F407ѧϰ��
	�Ա��꣺http://shop71381140.taobao.com/
	������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
****************************************************************************/

int main(void)
{
	Key_Init();
	LED_Init();
	LCD_Init();
	Touch_Init();
	COM1Init(115200);
	LCD_Clear(BLUE);
  	LCD_String(20,20,"Vido play!",RED);
	delay_ms(500);
	AVI_play();
  	while(1)
	{
	  	delay_ms(500);
		LEDTog(LED1);
	}
}


