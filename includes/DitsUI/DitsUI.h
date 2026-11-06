///@file    DitsUI.h
///@brief   TFT & Keypad user interface (UI) for RF-DITS protocol.
#pragma once
#include "Config.h"
#include <rf_dits.h>
#include <Adafruit_ST7735.h>
#include <OneSlot9pt7b.h>
#include "UIBase.h"
#include "UIRadio.h"
#include "UINode.h"
#include "UISetup.h"

class DitsUI
{
public:
  DitsUI::~DitsUI() {   }
  void Begin(DITSEngine& _dits, Adafruit_GFX& _gfx);
  void ButtonPress(int _Button);

private:
  DITSEngine* pDits = nullptr;
  Adafruit_GFX* pGFX = nullptr;
  void DumpRadio1DITS();
  byte NavNodeIdx = BNONE;
  UIBase* OnScreen = nullptr;
};