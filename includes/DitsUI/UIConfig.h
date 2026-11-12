#pragma once

// 1.8in ST7735S TFT is 128x160
#define TFT_WIDTH   128
#define TFT_HEIGHT  160


#define FONT_X      8           // Chosen Font xAdvance per character
#define FONT_Y      16          // Chosen Font yAdvance per line
#define TFT_VAL_X   100



// UI Navigation Screens & Navigation Commands
#define NCSETVALUE   0x00FF // Set a new value from OnScreen->SetValueWH and OnScreen->SetValue.
#define NCNEXTNODE   0x04FF
#define NCPREVNODE   0x05FF

#define NSNOCHANGE   0x02FF
#define NSNODE       0x03FF
#define NSSETUP      0x06FF
#define NSEDITTODE      0x07FF
// Below is prob NC but don't know yet.
#define UIADDDEV     0x09FF
#define UIDELNODE    0x0AFF
#define UIADDNODE    0x0BFF
#define UISYSTEM     0x0CFF

// UI Widget Handlers
#define WHNONE          0xFFFF
#define WHNODENAME      0x03FF
#define WHSECNET        0x04FF
#define WHRADIOID       0x05FF
#define WHNETID         0x06FF
#define WHRFADDR        0x07FF
//
/*
    Title   (0x01FF,1,      pGFX,TFT_COL_X(0),TFT_ROW_Y(0),F("Edit Tode")),
    LName   (0x02FF,0,      pGFX,TFT_COL_X(0),TFT_ROW_Y(1),F("Name:")),
    VName   (0x03FF,2,true, pGFX,TFT_COL_X(5),TFT_ROW_Y(1),11,nullptr),
    LSecNet (0x04FF,0,      pGFX,TFT_COL_X(0),TFT_ROW_Y(2),F("SecNet:")),
    VSecNet (0x05FF,3,true, pGFX,TFT_COL_X(7),TFT_ROW_Y(2),HEX,0x00,0x7F),
    LRadio  (0x06FF,0,      pGFX,TFT_COL_X(0),TFT_ROW_Y(3),F("Radio:")),
    VRadio  (0x07FF,0,false,pGFX,TFT_COL_X(6),TFT_ROW_Y(3)),
    LNetID  (0x08FF,0,      pGFX,TFT_COL_X(0),TFT_ROW_Y(4),F("NetID:")),
    VNetID  (0x09FF,4,true, pGFX,TFT_COL_X(6),TFT_ROW_Y(4)),
    LRFAddr (0x0AFF,0,      pGFX,TFT_COL_X(0),TFT_ROW_Y(5),F("RFAddr:")),
    VRFAddr (0x0BFF,5,true, pGFX,TFT_COL_X(7),TFT_ROW_Y(5))
    */

// Color Scheme
#define UI_FILLSCR_COLOR        ST77XX_BLACK
#define UI_SELCHAR_COLOR        ST77XX_BLUE
#define UI_STDCHAR_COLOR        ST77XX_WHITE
#define UI_STDLINE_COLOR        0x03E0

#define UIKB_NAMEEDIT_BGCOLOR   0x8410
#define UIKB_SELNAMECHAR_COLOR  0x001F
#define UIKB_STDNAMECHAR_COLOR  ST77XX_WHITE
#define UIKB_SELCHAR_COLOR      ST77XX_BLUE
#define UIKB_STDCHAR_COLOR      ST77XX_WHITE
#define GFXYELLOW 0xFFDE
 /*
 // Some ready-made 16-bit ('565') color settings:
 #define ST77XX_BLACK 0x0000
 #define ST77XX_WHITE 0xFFFF
 #define ST77XX_RED 0xF800
 #define ST77XX_GREEN 0x07E0
 #define ST77XX_BLUE 0x001F
 #define ST77XX_CYAN 0x07FF
 #define ST77XX_MAGENTA 0xF81F
 #define ST77XX_YELLOW 0xFFE0
 #define ST77XX_ORANGE 0xFC00
*/

/******************************************************************************//**
* @defgroup NAVKEY Navigate() Keypad Constants
* @{
*********************************************************************************/
// 0x[F]-Key Presses
#define NAVKEYNONE        0xFF
#define NAVKEYRIGHT       0xFE
#define NAVKEYLEFT        0xFD
#define NAVKEYUP          0xFC
#define NAVKEYDOWN        0xFB
#define NAVKEYSET         0xFA
#define NAVKEYGET        0xF9
#define KEYPRESSMIN       0xF0
///@}