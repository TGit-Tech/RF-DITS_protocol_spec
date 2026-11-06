#pragma once
#include "UIBase.h"

class UINode : public UIBase {
public:
    UINode(Adafruit_GFX& _GFX, DITSEngine& _DITS, byte _NodeIdx) : UIBase(_GFX), pDits(&_DITS), NodeIdx(_NodeIdx)  {}

    void Display() override {
      // Load Screen Data
      int row = 0;
      DITSEngine::NodeDIT node = pDits->Node(NodeIdx);
      DispRowIdx[row] = NodeIdx;
      node.NGetName(DispRowNames[row]);
      DITSEngine::DeviceDIT dev = pDits->Device();
      while (row < TFT_ROWS - 1 && dev.Next(node.NodeIdx())) {
        row++;
        DispRowIdx[row] = dev.DITidx();
        dev.DGetName(DispRowNames[row]);
      }
      
      // Draw the Screen
      pGFX->fillScreen(ST77XX_BLACK);
      pGFX->drawFastHLine(0, FONT_Y+2, TFT_WIDTH, 0x03E0);
      pGFX->drawFastVLine(TFT_VAL_X, FONT_Y+2, TFT_HEIGHT - FONT_Y, 0x03E0); 
      for (int r=0; r<TFT_ROWS; r++) {
        if (DispRowNames[r][0] == '\0') break;
        if(r==0) {pGFX->setTextColor(0x001F);} else {pGFX->setTextColor(ST77XX_WHITE);}
        pGFX->setCursor(0, TFT_ROW_Y(r));pGFX->print(DispRowNames[r]);
      }
    }
  
    byte ButtonPress(int _Button) override {
      DBUIAENTER((_Button),("DitsUI::ButtonPress\n"))
      if(_Button==NAVKEYRIGHT) { return UINEXTNODE; }
      if(_Button==NAVKEYLEFT) { return UIPREVNODE; }
      if((_Button==NAVKEYUP && SelectedRow > 0) || (_Button==NAVKEYDOWN && SelectedRow<TFT_ROWS-1 && DispRowNames[SelectedRow + 1][0] != '\0')) {
        
        // Set Current Selected Un-Highlighted.
        pGFX->setTextColor(ST77XX_WHITE);
        pGFX->setCursor(0, TFT_ROW_Y(SelectedRow));
        pGFX->print(DispRowNames[SelectedRow]);

        // Set New Selected Row Highlighted.
        if(_Button==NAVKEYUP) SelectedRow--;
        if(_Button==NAVKEYDOWN) SelectedRow++;
        pGFX->setTextColor(0x001F);
        pGFX->setCursor(0, TFT_ROW_Y(SelectedRow));
        pGFX->print(DispRowNames[SelectedRow]);
      }
      if(_Button==NAVKEYSET && SelectedRow==0) return UISETUP;
    }

private:
    DITSEngine* pDits;
    byte NodeIdx = 0;
    int DispRowIdx[TFT_ROWS] = {};
    char DispRowNames[TFT_ROWS][11] = {};
    int SelectedRow = 0;
};