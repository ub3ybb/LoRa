/*
	Modified library only for 128x160 display using 565 (16 bit) per color

*/
#ifndef __ST7735_NEW_H__
#define __ST7735_NEW_H__

#include "fonts.h"
#include <stdbool.h>
#include "main.h"

/****************************/

#define ST7735_RES_Pin GPIO_PIN_4
#define ST7735_RES_GPIO_Port GPIOC
#define ST7735_CS_Pin GPIO_PIN_0
#define ST7735_CS_GPIO_Port GPIOB
#define ST7735_DC_Pin GPIO_PIN_5
#define ST7735_DC_GPIO_Port GPIOC
// using SPI1 interface
#define ST7735_SPI_PORT hspi1
// using DMA1 channel 3

/****************************/

#define ST7735_MADCTL_MY 0x80
#define ST7735_MADCTL_MX 0x40
#define ST7735_MADCTL_MV 0x20
#define ST7735_MADCTL_ML 0x10
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH 0x04

/****************************/

// WaveShare ST7735S-based 1.8" display, rotate right

#define ST7735_WIDTH 160
#define ST7735_HEIGHT 128
#define ST7735_XSTART 1
#define ST7735_YSTART 2
#define ST7735_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_RGB)

/****************************/

#define ST7735_NOP 0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID 0x04
#define ST7735_RDDST 0x09

#define ST7735_SLPIN 0x10
#define ST7735_SLPOUT 0x11
#define ST7735_PTLON 0x12
#define ST7735_NORON 0x13

#define ST7735_INVOFF 0x20
#define ST7735_INVON 0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON 0x29
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
#define ST7735_RAMRD 0x2E

#define ST7735_PTLAR 0x30
#define ST7735_COLMOD 0x3A
#define ST7735_MADCTL 0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5

#define ST7735_RDID1 0xDA
#define ST7735_RDID2 0xDB
#define ST7735_RDID3 0xDC
#define ST7735_RDID4 0xDD

#define ST7735_PWCTR6 0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Color definitions
#define ST7735_BLACK 0x0000
#define ST7735_BLUE 0x001F
#define ST7735_RED 0xF800
#define ST7735_GREEN 0x07E0
#define ST7735_CYAN 0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW 0xFFE0
#define ST7735_WHITE 0xFFFF
#define ST7735_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

/****************************/

void ST7735_Init(void);

void ST7735_Start(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ST7735_Stop(void);

void ST7735_DrawPixel(uint8_t x, uint8_t y, uint16_t color);
void ST7735_WriteChar(uint8_t x, uint8_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor);
void ST7735_WriteString(uint8_t x, uint8_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor);
void ST7735_FillRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void ST7735_FillScreen(uint16_t color);
void ST7735_DrawImage(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *data);
void ST7735_InvertColors(bool invert);
void ST7735_WriteSerialStrings(const char *str, FontDef font, uint16_t color, uint16_t bgcolor);

#endif
