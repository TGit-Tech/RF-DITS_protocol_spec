#pragma once
#include "UIBase.h"

class UISetup : public UIBase {
private:
  const __FlashStringHelper* FMenuRows[7] = {};
  int SelectedRow = 0;
public:
    UISetup(Adafruit_GFX& _GFX) : UIBase(_GFX) {
      FMenuRows[0] = F("Setup Menu");
      FMenuRows[1] = F("Edit Tode Name");
      FMenuRows[2] = F("Radio Settings");
      FMenuRows[3] = F("Add Device");
      FMenuRows[4] = F("Delete Tode");
      FMenuRows[5] = F("Add Tode");
      FMenuRows[6] = F("System Settings");
      //Match count in private: const __FlashStringHelper* FMenuRows[7] = {};
    }

    void Display() override {
      
      pGFX->fillScreen(ST77XX_BLACK);
      pGFX->drawFastHLine(0, FONT_Y+2, TFT_WIDTH, 0x03E0);
      for (int r=0; r<TFT_ROWS && r < ArraySize(FMenuRows); r++) {
        if(r==0) {pGFX->setTextColor(0x001F);} else {pGFX->setTextColor(ST77XX_WHITE);}
        pGFX->setCursor(0, TFT_ROW_Y(r));pGFX->print(FMenuRows[r]);
      }
    }

    byte ButtonPress(int _Button) override {
      DBUIAENTER((_Button),("DitsUI::ButtonPress\n"))
      if(_Button==NAVKEYLEFT) { return UINODE;}
      if(_Button==NAVKEYUP && SelectedRow!=0) {
        // Set Current Selected Un-Highlighted.
        pGFX->setTextColor(ST77XX_WHITE);
        pGFX->setCursor(0, TFT_ROW_Y(SelectedRow));
        pGFX->print(FMenuRows[SelectedRow]);
      
        // Set New Selected Row Highlighted.
        SelectedRow--;
        pGFX->setTextColor(0x001F);
        pGFX->setCursor(0, TFT_ROW_Y(SelectedRow));
        pGFX->print(FMenuRows[SelectedRow]);
      }
      if(_Button==NAVKEYDOWN && SelectedRow < (TFT_ROWS-1) && (SelectedRow + 1) < ArraySize(FMenuRows)) {
        // Set Current Selected Un-Highlighted.
        pGFX->setTextColor(ST77XX_WHITE);
        pGFX->setCursor(0, TFT_ROW_Y(SelectedRow));
        pGFX->print(FMenuRows[SelectedRow]);
      
        // Set New Selected Row Highlighted.
        SelectedRow++;
        pGFX->setTextColor(0x001F);
        pGFX->setCursor(0, TFT_ROW_Y(SelectedRow));
        pGFX->print(FMenuRows[SelectedRow]);        
      }
      return UINOCHANGE;
    }
};