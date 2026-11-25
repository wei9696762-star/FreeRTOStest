#include "RTC.h"
#include "OLED.h"
#include "Key.h"

const uint8_t Return1[]={
	0x00,0x00,0x80,0xC0,0xE0,0xF0,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xF8,0xF8,0x00,
	0x00,0x01,0x03,0x07,0x0F,0x1F,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x00,
};//返回图标

RTC_DateTypeDef RTC_Date1;
RTC_TimeTypeDef RTC_Time1;

void Show_SetTime_FirstUI(void)
{
	OLED_Clear();
    OLED_ShowImageWithReverse(0,0,16,16,Return1,1);
    OLED_ShowCN(2,1,8,1); // 年
    OLED_ShowCN(3,1,9,1); // 月
    OLED_ShowCN(4,1,4,1); // 日
    OLED_ShowString(2,3,":",8);
    OLED_ShowString(3,3,":",8);
    OLED_ShowString(4,3,":",8);
    OLED_ShowNum(2,4,20,2,8); // 显示20xx
    
    HAL_RTC_GetDate(&hrtc,&RTC_Date1,RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc,&RTC_Time1,RTC_FORMAT_BIN);
    
    OLED_ShowNumWithReverse(2,6,RTC_Date1.Year,2,8,1);
    // 这里直接显示Month，没有任何减法
    OLED_ShowNumWithReverse(3,4,RTC_Date1.Month,2,8,1); 
    OLED_ShowNumWithReverse(4,4,RTC_Date1.Date,2,8,1);
}

void Show_SetTime_SecondUI(void)
{
    OLED_Clear();
    OLED_ShowCN(1,1,6,1);  // 时
    OLED_ShowCN(2,1,10,1); // 分
    OLED_ShowCN(3,1,11,1); // 秒
    OLED_ShowString(1,3,":",8);
    OLED_ShowString(2,3,":",8);
    OLED_ShowString(3,3,":",8);
    
    HAL_RTC_GetDate(&hrtc,&RTC_Date1,RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc,&RTC_Time1,RTC_FORMAT_BIN);
    
    OLED_ShowNumWithReverse(1,4,RTC_Time1.Hours,2,8,1);
    OLED_ShowNumWithReverse(2,4,RTC_Time1.Minutes,2,8,1);
    OLED_ShowNumWithReverse(3,4,RTC_Time1.Seconds,2,8,1);
}

uint8_t KeyNum1;
uint8_t KeyNum2;
uint8_t KeyNum3;
uint8_t KeyNum4;
uint8_t KeyNum5;
uint8_t KeyNum6;

// 1. 设置年份
int SetYear(void)
{
    while(1)
    {
        KeyNum1 = Key_GetNum();
        if(KeyNum1 == 1)      Change_RTC_Time(0, 1); 
        else if(KeyNum1 == 2) Change_RTC_Time(0, 0); 
        else if(KeyNum1 == 3) return 1;              

        Show_SetTime_FirstUI();
        OLED_ShowNumWithReverse(2, 6, MyRTC_Time[0], 2, 8, 0); 
        OLED_Update(); // Add Update here
    }
}

// 2. 设置月份 (完全正常逻辑)
int SetMonth(void)
{
    while(1)
    {
        KeyNum2 = Key_GetNum();
        if(KeyNum2 == 1)      Change_RTC_Time(1, 1); 
        else if(KeyNum2 == 2) Change_RTC_Time(1, 0); 
        else if(KeyNum2 == 3) return 1;

        Show_SetTime_FirstUI();
        // 直接显示调整后的月份 MyRTC_Time[1]，无需任何处理
        OLED_ShowNumWithReverse(3, 4, MyRTC_Time[1], 2, 8, 0);
        OLED_Update(); // Add Update here
    }
}

// 3. 设置日期
int SetDay(void)
{
    while(1)
    {
        KeyNum3 = Key_GetNum();
        if(KeyNum3 == 1)      Change_RTC_Time(2, 1); 
        else if(KeyNum3 == 2) Change_RTC_Time(2, 0); 
        else if(KeyNum3 == 3) return 1;

        Show_SetTime_FirstUI();
        OLED_ShowNumWithReverse(4, 4, MyRTC_Time[2], 2, 8, 0);
        OLED_Update(); // Add Update here
    }
}

// 4. 设置小时
int SetHour(void)
{
    while(1)
    {
        KeyNum4 = Key_GetNum();
        if(KeyNum4 == 1)      Change_RTC_Time(3, 1); 
        else if(KeyNum4 == 2) Change_RTC_Time(3, 0); 
        else if(KeyNum4 == 3) return 1;

        Show_SetTime_SecondUI(); 
        OLED_ShowNumWithReverse(1, 4, MyRTC_Time[3], 2, 8, 0);
        OLED_Update(); // Add Update here
    }
}

// 5. 设置分钟
int SetMin(void)
{
    while(1)
    {
        KeyNum5 = Key_GetNum();
        if(KeyNum5 == 1)      Change_RTC_Time(4, 1); 
        else if(KeyNum5 == 2) Change_RTC_Time(4, 0); 
        else if(KeyNum5 == 3) return 1;

        Show_SetTime_SecondUI();
        OLED_ShowNumWithReverse(2, 4, MyRTC_Time[4], 2, 8, 0);
        OLED_Update(); // Add Update here
    }
}

// 6. 设置秒
int SetSec(void)
{
    while(1)
    {
        KeyNum6 = Key_GetNum();
        if(KeyNum6 == 1)      Change_RTC_Time(5, 1); 
        else if(KeyNum6 == 2) Change_RTC_Time(5, 0); 
        else if(KeyNum6 == 3) return 1;

        Show_SetTime_SecondUI();
        OLED_ShowNumWithReverse(3, 4, MyRTC_Time[5], 2, 8, 0);
        OLED_Update(); // Add Update here
    }
}


uint8_t KeyNum;
int set_time_flag=1;
int set_time_flag1=0;

int SetTime(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t set_time_flag_temp=0;
		if(KeyNum==1)//上一项
		{
			set_time_flag--;
			if(set_time_flag<=0)set_time_flag=7;
		}
		else if(KeyNum==2)//下一项
		{
			set_time_flag++;
			if(set_time_flag>=8)set_time_flag=1;
		}
		else if(KeyNum==3)//确认
		{
			OLED_Clear();
			set_time_flag_temp=set_time_flag;
		}
		
		if(set_time_flag_temp==1)
        {
            set_time_flag=1;
            set_time_flag1=0;
            return 1;
        }
		else if(set_time_flag_temp==2)
        {
            SetYear();
            OLED_ShowNumWithReverse(2, 6, MyRTC_Time[0], 2, 8, 1);
        }//年
		else if(set_time_flag_temp==3)
        {
            SetMonth();
            OLED_ShowNumWithReverse(3, 4, MyRTC_Time[1], 2, 8, 1);
        }//月
		else if(set_time_flag_temp==4)
        {
            SetDay();
            OLED_ShowNumWithReverse(4, 4, MyRTC_Time[2], 2, 8, 1);
        }//日
		else if(set_time_flag_temp==5)
        {
            SetHour();
            OLED_ShowNumWithReverse(1, 4, MyRTC_Time[3], 2, 8, 1);
        }//时
		else if(set_time_flag_temp==6)
        {
            SetMin();
            OLED_ShowNumWithReverse(2, 4, MyRTC_Time[4], 2, 8, 1);
        }//分
		else if(set_time_flag_temp==7)
        {
            SetSec();
            OLED_ShowNumWithReverse(3, 4, MyRTC_Time[5], 2, 8, 1);
        }//秒

        // 下面是主界面的显示逻辑，已全部去掉月份特殊判断
        if(set_time_flag==1)
        {
            if(set_time_flag!=set_time_flag1&&set_time_flag1!=2)
            {
                Show_SetTime_FirstUI();
            }
            OLED_ShowImageWithReverse(0,0,16,16,Return1,0);
            OLED_ShowCN(2,1,8,1); // 年
            OLED_ShowCN(3,1,9,1); // 月
            OLED_ShowCN(4,1,4,1); // 日
            set_time_flag1=set_time_flag;
        }
        else if(set_time_flag==2)
        {
            OLED_ShowImageWithReverse(0,0,16,16,Return1,1);
            OLED_ShowCN(2,1,8,0); // 年
            OLED_ShowCN(3,1,9,1); // 月
            OLED_ShowCN(4,1,4,1); // 日
            set_time_flag1=set_time_flag;
        }
        else if(set_time_flag==3)
        {
            OLED_ShowImageWithReverse(0,0,16,16,Return1,1);
            OLED_ShowCN(2,1,8,1); // 年
            OLED_ShowCN(3,1,9,0); // 月
            OLED_ShowCN(4,1,4,1); // 日
            set_time_flag1=set_time_flag;
        }
        else if(set_time_flag==4)
        {
            if(set_time_flag1==5) Show_SetTime_FirstUI();
            OLED_ShowImageWithReverse(0,0,16,16,Return1,1);
            OLED_ShowCN(2,1,8,1); // 年
            OLED_ShowCN(3,1,9,1); // 月
            OLED_ShowCN(4,1,4,0); // 日
            set_time_flag1=set_time_flag;
        }
        else if(set_time_flag==5)
        {
            if(set_time_flag!=set_time_flag1&&set_time_flag1==4)
            {
                Show_SetTime_SecondUI();
            }
            OLED_ShowCN(1,1,6,0);  // 时
            OLED_ShowCN(2,1,10,1); // 分
            OLED_ShowCN(3,1,11,1); // 秒
            set_time_flag1=set_time_flag;
        }
        else if(set_time_flag==6)
        {
            OLED_ShowCN(1,1,6,1);  // 时
            OLED_ShowCN(2,1,10,0); // 分
            OLED_ShowCN(3,1,11,1); // 秒
            set_time_flag1=set_time_flag;
        }
        else if(set_time_flag==7)
        {
            if(set_time_flag1==1) Show_SetTime_SecondUI();
            OLED_ShowCN(1,1,6,1);  // 时
            OLED_ShowCN(2,1,10,1); // 分
            OLED_ShowCN(3,1,11,0); // 秒
            set_time_flag1=set_time_flag;
        }
        OLED_Update(); // Add Update here
	}
}