#include "st7735newLib.h"

#define DELAY 0x80

#define ST7735_COLOR_FIX(color) (color << 8) | ((color >> 8) & 0xFF);

// based on Adafruit ST7735 library for Arduino
static const uint8_t
init_cmds1[] = {            // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      ST7735_ROTATION,        //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color

init_cmds2[] = {            // Init for 7735R, part 2 (1.44" display)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F },           //     XEND = 127

init_cmds3[] = {            // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100 };                  //     100 ms delay

static uint16_t framebuffer[ST7735_WIDTH * ST7735_HEIGHT]; //-40 KB RAM AAAAAAAAA

uint16_t window_x0, window_x1, window_y0, window_y1; // already non static for GFX functions

uint8_t strNum = 0;

extern SPI_HandleTypeDef hspi1;

static void ST7735_Select() {
	ST7735_CS_GPIO_Port->BSRR = ST7735_CS_Pin << 16;
}

static void ST7735_Unselect() {
	ST7735_CS_GPIO_Port->BSRR = ST7735_CS_Pin;
}

static void ST7735_Reset() {
	ST7735_RES_GPIO_Port->BSRR = ST7735_RES_Pin << 16;
	HAL_Delay(5);
	ST7735_RES_GPIO_Port->BSRR = ST7735_RES_Pin;
}

static void ST7735_WriteCommand(uint8_t cmd) {
	ST7735_DC_GPIO_Port->BSRR = ST7735_DC_Pin << 16;
	HAL_SPI_Transmit(&ST7735_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
	SPI1->DR = cmd;
	while (!(SPI1->SR & 0x2))
		;
}

void ST7735_SetData(void) {
	ST7735_DC_GPIO_Port->BSRR = ST7735_DC_Pin;
}

static void ST7735_WriteData(uint8_t *buff, size_t buff_size) {
	ST7735_SetData();
	for (size_t i = 0; i < buff_size; i++) {
		SPI1->DR = buff[i];
		while (!(SPI1->SR & 0x2))
			;
	}
}

static void ST7735_ExecuteCommandList(const uint8_t *addr) {
	uint8_t numCommands, numArgs;
	uint16_t ms;

	numCommands = *addr++;
	while (numCommands--) {
		uint8_t cmd = *addr++;
		ST7735_WriteCommand(cmd);

		numArgs = *addr++;
		// If high bit set, delay follows args
		ms = numArgs & DELAY;
		numArgs &= ~DELAY;
		if (numArgs) {
			ST7735_WriteData((uint8_t *)addr, numArgs);
			addr += numArgs;
		}

		if (ms) {
			ms = *addr++;
			if (ms == 255)
				ms = 500;
			HAL_Delay(ms);
		}
	}
}

static void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
	// column address set
	ST7735_WriteCommand(ST7735_CASET);
	uint8_t data[] = {0x00, x0 + ST7735_XSTART, 0x00, x1 + ST7735_XSTART};
	ST7735_WriteData(data, sizeof(data));

	// row address set
	ST7735_WriteCommand(ST7735_RASET);
	data[1] = y0 + ST7735_YSTART;
	data[3] = y1 + ST7735_YSTART;
	ST7735_WriteData(data, sizeof(data));

	// write to RAM
	ST7735_WriteCommand(ST7735_RAMWR);
}

/////////////////////////////////////////////////////////////////////

void ST7735_Init() {
	ST7735_Select();
	ST7735_Reset();
	ST7735_ExecuteCommandList(init_cmds1);
	ST7735_ExecuteCommandList(init_cmds2);
	ST7735_ExecuteCommandList(init_cmds3);
	ST7735_Unselect();
	ST7735_Start(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);
	ST7735_FillRectangle(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1, ST7735_BLACK);
	// SPI and DMA init in circulur mode
}

void ST7735_Start(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
	if (x0 > x1) {
		window_x0 = x1;
		window_x1 = x0;
	} // I know you can do it
	else {
		window_x1 = x1;
		window_x0 = x0;
	}
	if (y0 > y1) {
		window_y0 = y1;
		window_y1 = y0;
	} else {
		window_y1 = y1;
		window_y0 = y0;
	}

	if (window_x1 - window_x0 + 1 > ST7735_WIDTH)
		window_x1 = ST7735_WIDTH - 1 - window_x0;
	if (window_y1 - window_y0 + 1 > ST7735_HEIGHT)
		window_y1 = ST7735_HEIGHT - 1 - window_y0;
	// Start DMA

	ST7735_Select();

	ST7735_SetAddressWindow(x0, y0, x1, y1);
	ST7735_SetData();
	HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)framebuffer, sizeof(uint16_t) * (window_x1 - window_x0 + 1) * (window_y1 - window_y0 + 1));
}

void ST7735_Stop(void) {
	// Stop DMA
	HAL_SPI_DMAStop(&hspi1);
	ST7735_Unselect();
}

/////////////////////////////////////////////////////////////////////
/* Further interaction with the display occurs through framebuffer */

void ST7735_DrawPixel(uint8_t x, uint8_t y, uint16_t color) {
	framebuffer[(window_x1 - window_x0 + 1) * y + x] = ST7735_COLOR_FIX(color);
}

void ST7735_WriteChar(uint8_t x, uint8_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {

	uint32_t i, b, j;

	for (i = 0; i < font.height; i++) {
		b = font.data[(ch - 32) * font.height + i];
		for (j = 0; j < font.width; j++) {
			if ((b << j) & 0x8000) {
				ST7735_DrawPixel(x + j, y + i, color);
			}
		}
	}
}

void ST7735_WriteString(uint8_t x, uint8_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor) {
	while (*str) {
		if (x + font.width >= window_x1 - window_x0 + 1) {
			x = 0;
			y += font.height;

			strNum++;

			if (y + font.height >= window_y1 - window_y0 + 1) {
				break;
			}

			if (*str == ' ') {
				// skip spaces in the beginning of the new line
				str++;
				continue;
			}
		}

		ST7735_WriteChar(x, y, *str, font, color, bgcolor);
		x += font.width;
		str++;
	}
}

void ST7735_FillRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
	if (x + w > window_x1 - window_x0 + 1)
		return;
	if (y + h > window_y1 - window_y0 + 1)
		return;

	for (size_t i = 0; i < h; i++) {
		for (size_t j = 0; j < w; j++) {
			framebuffer[(i + y) * (window_x1 - window_x0 + 1) + x + j] = ST7735_COLOR_FIX(color);
		}
	}
}

void ST7735_FillScreen(uint16_t color) {
	for (size_t i = 0; i < ST7735_WIDTH * ST7735_HEIGHT; i++)
		framebuffer[i] = ST7735_COLOR_FIX(color);
}

void ST7735_DrawImage(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *data) {
	if (x + w > window_x1 - window_x0 + 1)
		return;
	if (y + h > window_y1 - window_y0 + 1)
		return;

	for (size_t i = 0; i < h; i++) {
		for (size_t j = 0; j < w; j++) {
			framebuffer[(i + y) * (window_x1 - window_x0 + 1) + x + j] = data[i * h + j];
		}
	}
}

void ST7735_InvertColors(bool invert) {
	ST7735_Stop();
	ST7735_Select();
	ST7735_WriteCommand(invert ? ST7735_INVON : ST7735_INVOFF);
	ST7735_Unselect();
	ST7735_Start(window_x0, window_y0, window_x1, window_y1);
}


void ST7735_WriteSerialStrings(const char *str, FontDef font, uint16_t color, uint16_t bgcolor) {
	ST7735_FillRectangle(0, strNum * font.height + 7, ST7735_WIDTH, font.height, ST7735_BLACK);
	ST7735_WriteString(0, strNum * font.height + 7, str, font, color, bgcolor);
	strNum++;
	strNum = ((strNum * font.height + 7 + font.height) > ST7735_HEIGHT) ? 0 : strNum;
}