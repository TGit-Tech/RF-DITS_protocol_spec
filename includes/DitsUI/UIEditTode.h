#pragma once

#include "UIConfig.h"
#include "UIBase.h"

class UIEditTode : public UIBase {
private:
  WFLabel WFLTitle;
  WFLabel WFLName;
  WCharVal WCVName;
  WFLabel WFLSecNet;
  WNumVal WNVSecNet;
  WFLabel WFLRadio;
  WEnumVal WEVRadio;
  WFLabel WFLNetID;
  WNumVal WNVNetID;
  WFLabel WFLRFAddr;
  WNumVal WNVRFAddr;
  UIWidget* Widget[11] = {&WFLTitle, &WFLName, &WCVName, &WFLSecNet, &WNVSecNet, &WFLRadio, &WEVRadio, &WFLNetID, &WNVNetID, &WFLRFAddr, &WNVRFAddr};
  UIWidget* pSelected = nullptr; UIWidget* pEdit = nullptr;

public:
  UIEditTode(Adafruit_GFX& _GFX)
  : UIBase(_GFX),
    WFLTitle  (1, pGFX, TFT_COL_X(0), TFT_ROW_Y(0), F("Edit Tode")),
    WFLName   (0, pGFX, TFT_COL_X(0), TFT_ROW_Y(1), F("Name:")),
    WCVName   (2, pGFX, TFT_COL_X(5), TFT_ROW_Y(1), 11, true, WHNODENAME),
    WFLSecNet (0, pGFX, TFT_COL_X(0), TFT_ROW_Y(2), F("SecNet:")),
    WNVSecNet (3, pGFX, TFT_COL_X(7), TFT_ROW_Y(2), VDTBYTE, HEX, true, WHSECNET),
    WFLRadio  (0, pGFX, TFT_COL_X(0), TFT_ROW_Y(3), F("Radio:")),
    WEVRadio  (0, pGFX, TFT_COL_X(6), TFT_ROW_Y(3), false, WHRADIOID),
    WFLNetID  (0, pGFX, TFT_COL_X(0), TFT_ROW_Y(4), F("NetID:")),
    WNVNetID  (4, pGFX, TFT_COL_X(6), TFT_ROW_Y(4), VDTUINT, HEX, true, WHNETID),
    WFLRFAddr (0, pGFX, TFT_COL_X(0), TFT_ROW_Y(5), F("RFAddr:")),
    WNVRFAddr (5, pGFX, TFT_COL_X(7), TFT_ROW_Y(5), VDTUINT, HEX, true, WHRFADDR)
    {
      LastSelectNum = 5;
      NSID = NSEDITTODE;
      WEVRadio.AddEnum(0,F("NOTGOT"));
      WEVRadio.AddEnum(1,F("UNKNOWN"));
      WEVRadio.AddEnum(2,F("E22400T30D"));
      WEVRadio.AddEnum(3,F("E22400T33D"));
      WEVRadio.AddEnum(4,F("E22900T30D"));
      WEVRadio.AddEnum(5,F("E32433T20D"));
      WEVRadio.AddEnum(6,F("E32433T30D"));
      WEVRadio.AddEnum(7,F("E220400T30D"));
    }

    void DisplayScreen() override {
      if (DisplayedOnce) return;
      DisplayedOnce = true;
      pSelected = &WFLTitle; pSelected->Selected(true);
      pGFX->fillScreen(UI_FILLSCR_COLOR);
      pGFX->drawFastHLine(0, FONT_Y+2, TFT_WIDTH, UI_STDLINE_COLOR);
      for (int i=0; i<ArraySize(Widget); i++) {
        Widget[i]->Display();
      }
    }

    bool gotValue(uint16_t _NSID, uint16_t _WHandle, int _Value) override {
      if (_NSID!=NSID) return false;
      for (int i=0; i<ArraySize(Widget); i++) {
        if(Widget[i]->WHandle==_WHandle) {
          Widget[i]->Value(_Value); return true;
        }
      }
      return false;
    }
    bool gotValue(uint16_t _NSID, uint16_t _WHandle, char* _Value) override {
      if (_NSID!=NSID) return false;
      for (int i=0; i<ArraySize(Widget); i++) {
        if(Widget[i]->WHandle==_WHandle) {
          Widget[i]->chValue(_Value); return true;
        }
      } 
      return false; 
    }
    bool gotValue(uint16_t _NSID, uint16_t _WHandle, const char* _Value) override {
      if (_NSID!=NSID) return false;
      for (int i=0; i<ArraySize(Widget); i++) {
        if(Widget[i]->WHandle==_WHandle) {
          Widget[i]->chValue(_Value); return true;
        }
      } 
      return false;      
    }

    uint16_t ButtonPress(int _Button) override {
      DBUIAENTER((_Button),("DitsUI::ButtonPress\n"))
      if(_Button==NAVKEYLEFT) {
        if(pEdit!=nullptr) { return pEdit->EditButtonPress(_Button); }
        return NSNODE;
      }
      if(_Button==NAVKEYRIGHT) {
        if(pEdit!=nullptr) {return pEdit->EditButtonPress(_Button);}
        return NSNODE;
      }
      if(_Button==NAVKEYUP) {
        if(pEdit!=nullptr) {return pEdit->EditButtonPress(_Button);} // Edit Routing
        for (int s=pSelected->SelectNum - 1; s>0; s--) {
          for (int i=0; i<ArraySize(Widget); i++) {
            if(Widget[i]->SelectNum==s) {
              pSelected->Selected(false);
              pSelected = Widget[i];
              pSelected->Selected(true);
              return NSNOCHANGE;
            }
          }
        } 
      }
      if(_Button==NAVKEYDOWN) {
        if(pEdit!=nullptr) {return pEdit->EditButtonPress(_Button);}  // Edit Routing
        for (int s=pSelected->SelectNum + 1; s<=LastSelectNum; s++) {
          for (int i=0; i<ArraySize(Widget); i++) {
            if(Widget[i]->SelectNum==s) {
              pSelected->Selected(false);
              pSelected = Widget[i];
              pSelected->Selected(true);
              return NSNOCHANGE;
            }
          }
        }    
      }
      if(_Button==NAVKEYGET) {
        if(pEdit!=nullptr) {return pEdit->EditButtonPress(_Button);}  // Edit Routing
        //if(SelectedWidget==0) return NSEDITTODE;
      }
      if(_Button==NAVKEYSET) {
        // This Toggles Edit Mode
        if(pEdit==nullptr) { 
          if(pSelected->SetEditMode(true)) {
            pEdit = pSelected;
            pEdit->Display();
          }
        } else {
          pEdit->SetEditMode(false);
          pEdit->Display();
          SetValueWH = pEdit->WHandle;
          SetValue = pEdit->Value();
          chSetValue = pEdit->chValue();
          // Stage New Value and WHandle for Parent to process
          pEdit = nullptr;
          return NCSETVALUE;
        }
        return NSNOCHANGE;
      }
      return NSNOCHANGE;
    }
};