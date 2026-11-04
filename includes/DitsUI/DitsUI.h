///@file    DitsUI.h
///@brief   TFT & Keypad user interface (UI) for RF-DITS protocol.
#pragma once
#include "Config.h"
#include <rf_dits.h>
#include <Adafruit_ST7735.h>
#include <OneSlot9pt7b.h>

#define TFT_ROWS        (TFT_HEIGHT / FONT_Y)
#define TFT_ROW_Y(row)  (((row) + 1) * FONT_Y)

class DitsUI
{
public:
    void Begin(DITSEngine& _dits, Adafruit_GFX& _gfx);
    void DisplayNode();

private:
    DITSEngine* pDits = nullptr;
    Adafruit_GFX* pGFX = nullptr;
    void DumpRadio1DITS();
};

