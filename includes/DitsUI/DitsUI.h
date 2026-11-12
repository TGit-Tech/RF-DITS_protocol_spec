///@file    DitsUI.h
///@brief   TFT & Keypad user interface (UI) for RF-DITS protocol.
#pragma once

#include "Config.h"
#include <rf_dits.h>
#include <Adafruit_ST7735.h>
#include <OneSlot9pt7b.h>
#include <Ebyte.h>
#include "UIBase.h"     // Setup Base Class

// Screen Displays and Handlers
#include "UINode.h"       // Main Any Node Screen
#include "UISetup.h"      // Main Setup Screen
#include "UIRadio.h"      // |- Adv Radio Settings
#include "UIEditTode.h"

class DitsUI
{
public:
  DitsUI::~DitsUI() {   }
  void Begin(DITSEngine& _dits, Adafruit_GFX& _gfx, Ebyte& _pEbyte);
  void ButtonPress(int _Button);

private:
  DITSEngine* pDits = nullptr;
  Adafruit_GFX* pGFX = nullptr;
  Ebyte* pEbyte = nullptr;
  void DumpRadio1DITS();
  byte NavNodeIdx = BNONE;
  UIBase* OnScreen = nullptr;
  void SGValues(bool _Set = false);
  char* tmpDITName; // General use array for DIT get Names
};