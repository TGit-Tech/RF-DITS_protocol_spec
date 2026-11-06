#pragma once
#include <Adafruit_ST7735.h>

#define TFT_ROWS        (TFT_HEIGHT / FONT_Y)
#define TFT_ROW_Y(row)  (((row) + 1) * FONT_Y)

#define UINOCHANGE   0
#define UINODE       1
#define UINEXTNODE   2
#define UIPREVNODE   3
#define UISETUP      4
#define UINODENAME   5
#define UIRADIO      6
#define UIADDDEV     7
#define UIDELNODE    8
#define UIADDNODE    9
#define UISYSTEM     10
/******************************************************************************//**
* @defgroup NAVKEY Navigate() Keypad Constants
* @{
*********************************************************************************/
// 0x[F]-Key Presses
#ifndef NAVKEYNONE
  #define NAVKEYNONE        0xFF
#endif 
#ifndef NAVKEYRIGHT
  #define NAVKEYRIGHT       0xFE
#endif 
#ifndef NAVKEYLEFT
  #define NAVKEYLEFT        0xFD
#endif 
#ifndef NAVKEYUP
  #define NAVKEYUP          0xFC
#endif 
#ifndef NAVKEYDOWN
  #define NAVKEYDOWN        0xFB
#endif 
#ifndef NAVKEYSET
  #define NAVKEYSET         0xFA
#endif 
#ifndef NAVKEYOKAY
  #define NAVKEYOKAY        0xF9
#endif 
#ifndef KEYPRESSMIN
  #define KEYPRESSMIN       0xF0
#endif 
///@}

template<typename T, size_t N>
constexpr size_t ArraySize(T (&)[N]) { return N; }

class UIBase {
public:
    UIBase(Adafruit_GFX& _GFX) : pGFX(&_GFX) {}

    virtual ~UIBase() = default;

    virtual void Display() = 0;
    virtual byte ButtonPress(int _Button) = 0;

protected:
    Adafruit_GFX* pGFX = nullptr;
};