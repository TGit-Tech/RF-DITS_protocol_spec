#pragma once

// 1.8in ST7735S TFT is 128x160
#define TFT_WIDTH   128
#define TFT_HEIGHT  160


#define FONT_X      11
#define FONT_Y      16
#define TFT_VAL_X   100


#define TFT_ROWS        (TFT_HEIGHT / FONT_Y)
#define TFT_ROW_Y(row)  (((row) + 1) * FONT_Y)
