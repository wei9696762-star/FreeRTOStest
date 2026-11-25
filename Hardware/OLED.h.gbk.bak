#ifndef __OLED_H
#define __OLED_H
//字体大小：8表示8*16，6表示6*8,12表示12*24
//基本上都是基于行列坐标，若基于像素坐标会在注释中说明
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);//像素坐标
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char,uint8_t FontSize);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String,uint8_t FontSize);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length,uint8_t FontSize);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length,uint8_t FontSize);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length,uint8_t FontSize);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length,uint8_t FontSize);
//void oled_show_picture(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *pic, uint8_t mode);
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);//区域反色(此函数不太好用)
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image);//显示图像,像素坐标
void OLED_Printf(uint8_t Line, uint8_t Column, uint8_t FontSize, char *format, ...);//如需显示中文，请单独调用 OLED_ShowCN
void OLED_Update();
void OLED_ShowCharPixel(int16_t X, int16_t Y, char Char);//在指定像素位置显示一个字符(6x8大小)
void OLED_ShowNumPixel(int16_t X, int16_t Y, uint32_t Number, uint8_t Length);//在指定像素位置显示数字(6x8大小)

extern uint8_t OLED_Buffer[8][128];
//带反色显示的函数
uint8_t OLED_ShowCharWithReverse(uint8_t Line, uint8_t Column, char Char, uint8_t FontSize, uint8_t mode);
uint8_t OLED_ShowStringWithReverse(uint8_t Line, uint8_t Column, char *String, uint8_t FontSize, uint8_t mode);
uint8_t OLED_ShowNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode);
uint8_t OLED_ShowSignedNumWithReverse(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode);
uint8_t OLED_ShowHexNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode);
uint8_t OLED_ShowBinNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode);
uint8_t OLED_ShowCN(uint8_t Line, uint8_t Column, uint8_t Num, uint8_t mode);//显示中文
uint8_t OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp, uint8_t mode);//显示图片
uint8_t OLED_ShowImageWithReverse(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image, uint8_t mode);
//绘图函数(基于像素坐标)
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t mode);
uint8_t OLED_GetPoint(int16_t X, int16_t Y);
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled);
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);
#endif
