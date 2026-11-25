#include "OLED_Font.h"
#include "gpio.h"
#include <string.h> // 需要用到 memset
#include <stdio.h>
#include <stdarg.h>
#include <math.h>   // 用于圆弧和椭圆计算
#include <stdlib.h> // 用于abs函数
#define OLED_W_SCL(x)	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,x)
#define OLED_W_SDA(x)	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,x)

uint8_t OLED_Buffer[8][128]; // 显存缓冲区，8页，每页128列
static uint8_t current_page = 0;    // 软件光标：当前页
static uint8_t current_col = 0;     // 软件光标：当前列

/*引脚初始化*/
void OLED_I2C_Init(void)
{
    MX_GPIO_Init();
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C开始
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);
	OLED_W_SCL(0);
}

/**
  * @brief  I2C停止
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C发送一个字节
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(Byte & (0x80 >> i));
		OLED_W_SCL(1);
		OLED_W_SCL(0);
	}
	OLED_W_SCL(1);	//额外的一个时钟，不处理应答信号
	OLED_W_SCL(0);
}

/**
  * @brief  OLED写命令 (直接操作硬件)
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x00);		//写命令
	OLED_I2C_SendByte(Command); 
	OLED_I2C_Stop();
}

/**
  * @brief  【核心修改】OLED更新显存到屏幕
  * @note   调用此函数后，屏幕才会显示画面
  */
void OLED_Update(void)
{
    uint8_t i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_WriteCommand(0xB0 + i);    // 设置页地址
        OLED_WriteCommand(0x00);        // 设置列地址低4位
        OLED_WriteCommand(0x10);        // 设置列地址高4位
        
        OLED_I2C_Start();
        OLED_I2C_SendByte(0x78);
        OLED_I2C_SendByte(0x40);        // 写数据模式
        
        for (n = 0; n < 128; n++)
        {
            OLED_I2C_SendByte(OLED_Buffer[i][n]);
        }
        OLED_I2C_Stop();
    }
}

// 兼容性宏，防止你的代码报错，虽然叫Refresh但实际调用Update
void OLED_Refresh(void) {
    OLED_Update();
}

/**
  * @brief  【修改】仅写入缓冲区，不发送I2C
  */
void OLED_WriteData(uint8_t Data)
{
    // 更新缓冲区
    if (current_page < 8 && current_col < 128) {
        OLED_Buffer[current_page][current_col] = Data;
    }
    
    // 自动递增列，模仿硬件行为
    current_col++;
    if (current_col >= 128) {
        current_col = 0; // 列地址回绕（虽然实际硬件可能跳下一页，但这里简单回绕即可）
    }
}

/**
  * @brief  【修改】仅设置软件光标，不发送I2C
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    if (Y < 8) current_page = Y;
    if (X < 128) current_col = X;
}

/**
  * @brief  【修改】OLED清屏 (清空缓冲区)
  * @note   需要调用 OLED_Update() 才会生效
  */
void OLED_Clear(void)
{  
    // 使用 memset 快速清零缓冲区
    memset(OLED_Buffer, 0, sizeof(OLED_Buffer));
    // 重置光标
    current_page = 0;
    current_col = 0;
}

/**
  * @brief  清除OLED显存指定区域（将区域内的像素熄灭）
  * @param  X      起始横坐标
  * @param  Y      起始纵坐标
  * @param  Width  区域宽度
  * @param  Height 区域高度
  * @note   需要调用 OLED_Update() 后才会生效
  */
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    int16_t i, j;
    
    // 遍历指定矩形区域的每一个像素
    for (j = Y; j < Y + Height; j++) // 循环Y轴
    {
        for (i = X; i < X + Width; i++) // 循环X轴
        {
            // 边界保护，防止数组越界
            if (i >= 0 && i <= 127 && j >= 0 && j <= 63)
            {
                // j / 8 计算页地址 (0-7)
                // j % 8 计算位偏移 (0-7)
                // &= ~(1<<n) 是标准的位清除操作，只灭掉这一个点，不影响同列的其他点
                OLED_Buffer[j / 8][i] &= ~(0x01 << (j % 8));
            }
        }
    }
}

/**
  * 函    数：将OLED显存数组部分取反
  */
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    int16_t i, j;
    // 仅修改缓冲区
    for (j = Y; j < Y + Height; j++)
    {
        for (i = X; i < X + Width; i++)
        {
            if (i >= 0 && i <= 127 && j >= 0 && j <= 63)
            {
                OLED_Buffer[j / 8][i] ^= (0x01 << (j % 8));
            }
        }
    }
}

/**
  * @brief  OLED显示一个字符
  * @note   已适配缓冲区模式
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char,uint8_t FontSize)
{      	
	uint8_t i;
    if(FontSize == 8) // 8*16字体大小
    {
        OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);        
        for (i = 0; i < 8; i++)
        {
            OLED_WriteData(OLED_F8x16[Char - ' '][i]);           
        }
        OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);    
        for (i = 0; i < 8; i++)
        {
            OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);       
        }
    }
    else if(FontSize == 6) // 6*8字体大小
    {
        OLED_SetCursor(Line - 1, (Column - 1) * 6);              
        for (i = 0; i < 6; i++)
        {
            OLED_WriteData(OLED_F6x8[Char - ' '][i]);            
        }
    }
    else if(FontSize == 12) // 12*24字体大小
    {
        OLED_SetCursor((Line - 1) * 3, (Column - 1) * 12);       
        for (i = 0; i < 12; i++)
        {
            OLED_WriteData(OLED_F12x24[Char - ' '][i]);          
        }
        
        OLED_SetCursor((Line - 1) * 3 + 1, (Column - 1) * 12);   
        for (i = 0; i < 12; i++)
        {
            OLED_WriteData(OLED_F12x24[Char - ' '][i + 12]);     
        }
        
        OLED_SetCursor((Line - 1) * 3 + 2, (Column - 1) * 12);   
        for (i = 0; i < 12; i++)
        {
            OLED_WriteData(OLED_F12x24[Char - ' '][i + 24]);     
        }
    }
}

/**
  * @brief  OLED显示一个字符(带反色)
  */
uint8_t OLED_ShowCharWithReverse(uint8_t Line, uint8_t Column, char Char, uint8_t FontSize, uint8_t mode)
{      	
	uint8_t i;
    uint8_t temp;
    
    if(FontSize == 8)
    {
        OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
        for (i = 0; i < 8; i++)
        {
            temp = OLED_F8x16[Char - ' '][i];
            OLED_WriteData(mode ? temp : ~temp); 
        }
        OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
        for (i = 0; i < 8; i++)
        {
            temp = OLED_F8x16[Char - ' '][i + 8];
            OLED_WriteData(mode ? temp : ~temp);
        }
    }
    else if(FontSize == 6)
    {
        OLED_SetCursor(Line - 1, (Column - 1) * 6);
        for (i = 0; i < 6; i++)
        {
            temp = OLED_F6x8[Char - ' '][i];
            OLED_WriteData(mode ? temp : ~temp);
        }
    }
    else if(FontSize == 12)
    {
        OLED_SetCursor((Line - 1) * 3, (Column - 1) * 12);
        for (i = 0; i < 12; i++)
        {
            temp = OLED_F12x24[Char - ' '][i];
            OLED_WriteData(mode ? temp : ~temp);
        }
        
        OLED_SetCursor((Line - 1) * 3 + 1, (Column - 1) * 12);
        for (i = 0; i < 12; i++)
        {
            temp = OLED_F12x24[Char - ' '][i + 12];
            OLED_WriteData(mode ? temp : ~temp);
        }
        
        OLED_SetCursor((Line - 1) * 3 + 2, (Column - 1) * 12);
        for (i = 0; i < 12; i++)
        {
            temp = OLED_F12x24[Char - ' '][i + 24];
            OLED_WriteData(mode ? temp : ~temp);
        }
    }
    return mode;
}

/**
  * @brief  OLED显示字符串
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String,uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i],FontSize);
	}
}

/**
  * @brief  OLED显示字符串(带反色)
  */
uint8_t OLED_ShowStringWithReverse(uint8_t Line, uint8_t Column, char *String, uint8_t FontSize, uint8_t mode)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowCharWithReverse(Line, Column + i, String[i], FontSize, mode);
	}
    return mode;
}

/**
  * @brief  OLED次方函数
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length,uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0',FontSize);
	}
}

/**
  * @brief  OLED显示带符号数字
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length,uint8_t FontSize)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+',FontSize);
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-',FontSize);
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0',FontSize);
	}
}

/**
  * @brief  OLED显示十六进制
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length,uint8_t FontSize)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0',FontSize);
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A',FontSize);
		}
	}
}

/**
  * @brief  OLED显示二进制
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length,uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0',FontSize);
	}
}

// 各种WithReverse函数保持原样逻辑，因为它们最终调用的是ShowCharWithReverse，而那个函数已经适配了缓冲

uint8_t OLED_ShowNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowCharWithReverse(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize, mode);
	}
    return mode;
}

uint8_t OLED_ShowSignedNumWithReverse(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowCharWithReverse(Line, Column, '+', FontSize, mode);
		Number1 = Number;
	}
	else
	{
		OLED_ShowCharWithReverse(Line, Column, '-', FontSize, mode);
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowCharWithReverse(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize, mode);
	}
    return mode;
}

uint8_t OLED_ShowHexNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowCharWithReverse(Line, Column + i, SingleNumber + '0', FontSize, mode);
		}
		else
		{
			OLED_ShowCharWithReverse(Line, Column + i, SingleNumber - 10 + 'A', FontSize, mode);
		}
	}
    return mode;
}

uint8_t OLED_ShowBinNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowCharWithReverse(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize, mode);
	}
    return mode;
}

/**
  * @brief  OLED初始化
  */
void OLED_Init(void)
{
	uint32_t i, j;
	
	for (i = 0; i < 1000; i++)			//上电延时
	{
		for (j = 0; j < 1000; j++);
	}
	
	OLED_I2C_Init();			//端口初始化
	
	OLED_WriteCommand(0xAE);	//关闭显示
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	OLED_WriteCommand(0x40);	//设置显示开始行
	OLED_WriteCommand(0xA1);	//设置左右方向
	OLED_WriteCommand(0xC8);	//设置上下方向
	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);
	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);
	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);
	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭
	OLED_WriteCommand(0xA6);	//设置正常/倒转显示
	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);
	OLED_WriteCommand(0xAF);	//开启显示
		
	OLED_Clear();				//清空缓冲区
    OLED_Update();              //刷新屏幕（全黑）
}

/**
 * @brief       OLED显示汉字
 */
uint8_t OLED_ShowCN(uint8_t Line, uint8_t Column, uint8_t Num, uint8_t mode)  
{      
    uint8_t i;
    uint8_t page_start = (Line - 1) * 2;   
    uint8_t col_start = (Column - 1) * 16; 

    OLED_SetCursor(page_start, col_start);
    for (i = 0; i < 16; i++) 
    {
        uint8_t data = OLED_F10x16[Num][i];
        OLED_WriteData(mode ? data : ~data); 
    }

    OLED_SetCursor(page_start + 1, col_start);
    for (i = 16; i < 32; i++) 
    {
        uint8_t data = OLED_F10x16[Num][i];
        OLED_WriteData(mode ? data : ~data); 
    }
    return mode;
}

/**
  * @brief  OLED画点函数
  */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t mode)
{
    if (x >= 128 || y >= 64) return; // 坐标越界处理
    
    uint8_t page = y / 8;      // 计算页地址
    uint8_t bit_pos = y % 8;   // 计算页内位位置
    
    // 修改缓冲区对应位
    if (mode) {
        OLED_Buffer[page][x] |= (1 << bit_pos);  // 点亮像素
    } else {
        OLED_Buffer[page][x] &= ~(1 << bit_pos); // 熄灭像素
    }
    // 注意：这里不再立即调用OLED_WriteData
}

/**
  * @brief  OLED显示位图
  */
uint8_t OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp, uint8_t mode) 
{
    uint8_t page_end = y + height / 8;
    uint8_t col_end = x + width;
    uint16_t index = 0;
    
    for (uint8_t page = y; page < page_end; page++) 
    {
        OLED_SetCursor(page, x);
        for (uint8_t col = x; col < col_end; col++) 
        {
            uint8_t data = bmp[index++];
            OLED_WriteData(mode ? data : ~data); 
        }
    }
    return mode;
}

/**
  * 函    数：OLED显示图像（仅修改缓冲区）
  */
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
    uint8_t i = 0, j = 0;
    int16_t Page, Shift;
    
    /*遍历指定图像涉及的相关页*/
    for (j = 0; j < ((Height - 1) / 8 + 1); j++)
    {
        /*遍历指定图像涉及的相关列*/
        for (i = 0; i < Width; i++)
        {
            int16_t screen_x = X + i;
            if (screen_x >= 0 && screen_x <= 127)    
            {
                Page = Y / 8;
                Shift = Y % 8;
                
                uint16_t image_index = j * Width + i;
                
                if (Page + j >= 0 && Page + j <= 7)    
                {
                    if (image_index < ((Height * Width) / 8)) {
                        OLED_Buffer[Page + j][screen_x] |= Image[image_index] << Shift;
                    }
                }
                
                if (Page + j + 1 >= 0 && Page + j + 1 <= 7)    
                {                    
                    if (image_index < ((Height * Width) / 8)) {
                        OLED_Buffer[Page + j + 1][screen_x] |= Image[image_index] >> (8 - Shift);
                    }
                }
            }
        }
    }
    // 删除了底部的I2C Update部分，由主循环统一调用OLED_Update
}

/**
  * 函    数：OLED显示图像（反色支持，仅修改缓冲区）
  */
uint8_t OLED_ShowImageWithReverse(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image, uint8_t mode)
{
    uint8_t i = 0, j = 0;
    int16_t Page, Shift;
    
    for (j = 0; j < ((Height - 1) / 8 + 1); j++)
    {
        for (i = 0; i < Width; i++)
        {
            int16_t screen_x = X + i;
            if (screen_x >= 0 && screen_x <= 127)
            {
                Page = Y / 8;
                Shift = Y % 8;
                uint16_t image_index = j * Width + i;
                
                uint8_t current_byte = 0;
                if (image_index < ((Height * Width) / 8)) {
                    current_byte = Image[image_index];
                }

                if (mode == 0) {
                    current_byte = ~current_byte;
                }

                if (Page + j >= 0 && Page + j <= 7)
                {
                    OLED_Buffer[Page + j][screen_x] &= ~(0xFF << Shift);
                    OLED_Buffer[Page + j][screen_x] |= (current_byte << Shift);
                }
                
                if (Page + j + 1 >= 0 && Page + j + 1 <= 7)
                {                    
                    OLED_Buffer[Page + j + 1][screen_x] &= ~(0xFF >> (8 - Shift));
                    OLED_Buffer[Page + j + 1][screen_x] |= (current_byte >> (8 - Shift));
                }
            }
        }
    }
    return mode;
}

/**
  * 函    数：OLED使用printf函数打印格式化字符串
  * 参    数：Line     行号 (1~4 for 8x16, 1~8 for 6x8)
  * 参    数：Column   列号 (1~16 for 8x16, 1~21 for 6x8)
  * 参    数：FontSize 字体大小 (8 或 6)
  * 参    数：format   格式化字符串
  * 注    意：如需显示中文，请单独调用 OLED_ShowCN。最后要加上OLED_Update()更新
  */
void OLED_Printf(uint8_t Line, uint8_t Column, uint8_t FontSize, char *format, ...)
{
    char String[256];
    va_list arg;
    uint16_t i = 0;
    
    // --- 1. 坐标转换核心逻辑 ---
    // 将 Line/Column 转换为 像素坐标 X/Y
    // 逻辑参考了你原本的 OLED_ShowChar
    int16_t X_Pixel, Y_Pixel;
    
    if (FontSize == 8) // 8x16 字体
    {
        X_Pixel = (Column - 1) * 8;
        Y_Pixel = (Line - 1) * 16;
    }
    else // 6x8 字体
    {
        X_Pixel = (Column - 1) * 6;
        Y_Pixel = (Line - 1) * 8;
    }

    // --- 2. 格式化字符串 ---
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);

    // --- 3. 绘制字符 ---
    while (String[i] != '\0')
    {
        // 处理换行 (如果字符串里有 \n，自动换到下一行像素位置)
        if (String[i] == '\n') 
        {
            Y_Pixel += (FontSize == 8) ? 16 : 8;
            X_Pixel = (Column - 1) * (FontSize == 8 ? 8 : 6); // 回车后回到起始列
            i++;
            continue;
        }

        // 跳过中文 (防止乱码)
        if ((uint8_t)String[i] > 127)
        {
            i += 2;
            continue; 
        }

        // 绘制 ASCII
        const uint8_t *fontData;
        uint8_t w, h;
        uint8_t cx, cy;
        
        if (FontSize == 8) {
            fontData = OLED_F8x16[String[i] - ' '];
            w = 8; h = 16;
        } else {
            fontData = OLED_F6x8[String[i] - ' '];
            w = 6; h = 8;
        }

        if (X_Pixel + w > 128) break; // 超出屏幕宽度停止

        for (cx = 0; cx < w; cx++)
        {
            uint16_t colData = 0;
            if (h == 16) colData = fontData[cx] | (fontData[cx + 8] << 8);
            else colData = fontData[cx];

            for (cy = 0; cy < h; cy++)
            {
                if (Y_Pixel + cy >= 64) break; 
                
                if ((colData >> cy) & 0x01) OLED_DrawPoint(X_Pixel + cx, Y_Pixel + cy, 1);
                else OLED_DrawPoint(X_Pixel + cx, Y_Pixel + cy, 0);
            }
        }
        
        X_Pixel += w;
        i++;
    }
}

/**
  * @brief  获取显存中某点的状态
  * @retval 0:灭, 1:亮
  */
uint8_t OLED_GetPoint(int16_t X, int16_t Y)
{
    if (X < 0 || X > 127 || Y < 0 || Y > 63) return 0;
    return (OLED_Buffer[Y / 8][X] >> (Y % 8)) & 0x01;
}

/**
  * @brief  画线 (Bresenham算法)
  */
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1)
{
    int16_t x = X0;
    int16_t y = Y0;
    int16_t dx = abs(X1 - X0);
    int16_t dy = abs(Y1 - Y0);
    int8_t sx = (X0 < X1) ? 1 : -1;
    int8_t sy = (Y0 < Y1) ? 1 : -1;
    int16_t err = dx - dy;
    int16_t e2;

    while (1)
    {
        OLED_DrawPoint(x, y, 1);
        if (x == X1 && y == Y1) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

/**
  * @brief  画矩形
  */
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
    if (IsFilled)
    {
        uint8_t i, j;
        for (i = 0; i < Height; i++)
        {
            // 优化：可以直接操作缓冲区，但为了安全这里循环调用画线或画点
            // 此处直接画横线填充
            OLED_DrawLine(X, Y + i, X + Width - 1, Y + i);
        }
    }
    else
    {
        OLED_DrawLine(X, Y, X + Width - 1, Y);                      // 上边
        OLED_DrawLine(X, Y + Height - 1, X + Width - 1, Y + Height - 1); // 下边
        OLED_DrawLine(X, Y, X, Y + Height - 1);                     // 左边
        OLED_DrawLine(X + Width - 1, Y, X + Width - 1, Y + Height - 1); // 右边
    }
}

/**
  * @brief  画圆 (中点圆算法)
  */
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled)
{
    int16_t x = 0;
    int16_t y = Radius;
    int16_t d = 3 - 2 * Radius;

    while (x <= y)
    {
        if (IsFilled)
        {
            // 填充模式：画横线连接对称点
            OLED_DrawLine(X - x, Y + y, X + x, Y + y);
            OLED_DrawLine(X - y, Y + x, X + y, Y + x);
            OLED_DrawLine(X - x, Y - y, X + x, Y - y);
            OLED_DrawLine(X - y, Y - x, X + y, Y - x);
        }
        else
        {
            // 轮廓模式：画八个对称点
            OLED_DrawPoint(X + x, Y + y, 1);
            OLED_DrawPoint(X - x, Y + y, 1);
            OLED_DrawPoint(X + x, Y - y, 1);
            OLED_DrawPoint(X - x, Y - y, 1);
            OLED_DrawPoint(X + y, Y + x, 1);
            OLED_DrawPoint(X - y, Y + x, 1);
            OLED_DrawPoint(X + y, Y - x, 1);
            OLED_DrawPoint(X - y, Y - x, 1);
        }

        if (d < 0) d = d + 4 * x + 6;
        else
        {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

/**
  * @brief  画三角形 (填充采用包围盒法，简单稳定)
  */
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled)
{
    if (IsFilled)
    {
        // 寻找包围盒
        int16_t minX = X0 < X1 ? (X0 < X2 ? X0 : X2) : (X1 < X2 ? X1 : X2);
        int16_t maxX = X0 > X1 ? (X0 > X2 ? X0 : X2) : (X1 > X2 ? X1 : X2);
        int16_t minY = Y0 < Y1 ? (Y0 < Y2 ? Y0 : Y2) : (Y1 < Y2 ? Y1 : Y2);
        int16_t maxY = Y0 > Y1 ? (Y0 > Y2 ? Y0 : Y2) : (Y1 > Y2 ? Y1 : Y2);

        // 限制在屏幕范围内
        if (minX < 0) minX = 0; if (maxX > 127) maxX = 127;
        if (minY < 0) minY = 0; if (maxY > 63) maxY = 63;

        for (int16_t y = minY; y <= maxY; y++)
        {
            for (int16_t x = minX; x <= maxX; x++)
            {
                // 使用叉乘判断点是否在三角形内
                int16_t edge0 = (X1 - X0) * (y - Y0) - (Y1 - Y0) * (x - X0);
                int16_t edge1 = (X2 - X1) * (y - Y1) - (Y2 - Y1) * (x - X1);
                int16_t edge2 = (X0 - X2) * (y - Y2) - (Y0 - Y2) * (x - X2);

                // 如果三个叉乘结果符号相同（或为0），则点在三角形内
                if ((edge0 >= 0 && edge1 >= 0 && edge2 >= 0) || 
                    (edge0 <= 0 && edge1 <= 0 && edge2 <= 0))
                {
                    OLED_DrawPoint(x, y, 1);
                }
            }
        }
    }
    else
    {
        OLED_DrawLine(X0, Y0, X1, Y1);
        OLED_DrawLine(X1, Y1, X2, Y2);
        OLED_DrawLine(X2, Y2, X0, Y0);
    }
}

/**
  * @brief  画椭圆
  */
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
    int16_t x = 0, y = B;
    int32_t a2 = A * A, b2 = B * B;
    int32_t d = b2 - a2 * B + a2 / 4;

    // 区域1
    while (b2 * x <= a2 * y)
    {
        if (IsFilled)
        {
            OLED_DrawLine(X - x, Y + y, X + x, Y + y);
            OLED_DrawLine(X - x, Y - y, X + x, Y - y);
        }
        else
        {
            OLED_DrawPoint(X + x, Y + y, 1);
            OLED_DrawPoint(X - x, Y + y, 1);
            OLED_DrawPoint(X + x, Y - y, 1);
            OLED_DrawPoint(X - x, Y - y, 1);
        }

        if (d < 0) d += b2 * (2 * x + 3);
        else { d += b2 * (2 * x + 3) + a2 * (-2 * y + 2); y--; }
        x++;
    }

    // 区域2
    d = b2 * (x * x + x) + a2 * (y * y - y) - a2 * b2 / 4 + (b2 + a2) / 2; // 近似初始值
    // 更精确的初始值: d = b2*(x+0.5)^2 + a2*(y-1)^2 - a2*b2
    
    while (y >= 0)
    {
        if (IsFilled)
        {
            OLED_DrawLine(X - x, Y + y, X + x, Y + y);
            OLED_DrawLine(X - x, Y - y, X + x, Y - y);
        }
        else
        {
            OLED_DrawPoint(X + x, Y + y, 1);
            OLED_DrawPoint(X - x, Y + y, 1);
            OLED_DrawPoint(X + x, Y - y, 1);
            OLED_DrawPoint(X - x, Y - y, 1);
        }
        
        if (d < 0) { d += b2 * (2 * x + 2) + a2 * (-2 * y + 3); x++; }
        else d += a2 * (-2 * y + 3);
        y--;
    }
}

/**
  * @brief  画圆弧 (需要 math.h)
  * @param  StartAngle: 起始角度 (0-360)
  * @param  EndAngle:   结束角度 (0-360)
  */
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
    float angle;
    int16_t x_curr, y_curr;
    int16_t x_pre, y_pre;
    uint8_t first_point = 1;
    
    #ifndef M_PI
    #define M_PI 3.14159265358979323846
    #endif

    // 步长为1度，保证精度
    for (int16_t i = StartAngle; i <= EndAngle; i++)
    {
        angle = (float)i * M_PI / 180.0f;
        x_curr = X + (int16_t)(Radius * cos(angle));
        y_curr = Y + (int16_t)(Radius * sin(angle));

        if (IsFilled)
        {
            // 填充模式：画扇形，从圆心连接到圆弧点
            OLED_DrawLine(X, Y, x_curr, y_curr);
        }
        else
        {
            // 轮廓模式：为了防止点之间有空隙，使用线段连接上一个点和当前点
            if (!first_point)
            {
                OLED_DrawLine(x_pre, y_pre, x_curr, y_curr);
            }
            else
            {
                OLED_DrawPoint(x_curr, y_curr, 1);
                first_point = 0;
            }
            x_pre = x_curr;
            y_pre = y_curr;
        }
    }
}

// 在指定像素位置显示一个字符 (6x8大小)
void OLED_ShowCharPixel(int16_t X, int16_t Y, char Char)
{
    uint8_t i, b;
    uint8_t c = Char - ' '; // 得到偏移值
    
    for (i = 0; i < 6; i++) // 遍历6列
    {
        uint8_t temp = OLED_F6x8[c][i];
        for (b = 0; b < 8; b++) // 遍历8行
        {
            // 如果超出屏幕范围则不画
            if ((X + i) > 127 || (Y + b) > 63) continue;
            
            if (temp & (1 << b)) 
            {
                OLED_DrawPoint(X + i, Y + b, 1); // 点亮
            }
            else 
            {
                OLED_DrawPoint(X + i, Y + b, 0); // 熄灭（擦除背景，防止重叠）
            }
        }
    }
}

// 在指定像素位置显示数字 (调用上面的函数)
void OLED_ShowNumPixel(int16_t X, int16_t Y, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        // 调用你原本就有的 OLED_Pow 函数计算每一位的值
        uint8_t single_num = Number / OLED_Pow(10, Length - i - 1) % 10;
        OLED_ShowCharPixel(X + (i * 6), Y, single_num + '0');
    }
}
