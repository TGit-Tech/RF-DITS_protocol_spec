#pragma once

#include "Config.h"
#include <rf_dits.h>
#include "UIBase.h"
//#include "UIPerChValEdit.h"

class UIEditTode : public UIBase {
private:
  char ptest[11] = "TestName";
  uint16_t testval = 0xFFF0;

  //UIPerChValEdit* pEdit = nullptr;

  UIFLabel Title;
  UIFLabel LName;
  UICharVal VName;
  UIFLabel LSecNet;
  UIByteVal VSecNet;
  UIFLabel LRadio;
  UIEnumVal VRadio;
  UIFLabel LNetID;
  UIUIntVal VNetID;
  UIFLabel LRFAddr;
  UIUIntVal VRFAddr;
  UIElement* Elmt[11] = {&Title, &LName, &VName, &LSecNet, &VSecNet, &LRadio, &VRadio, &LNetID, &VNetID, &LRFAddr, &VRFAddr};
  UIElement* pSelected = nullptr;
  UIElement* pEdit = nullptr;

public:
  UIEditTode(Adafruit_GFX& _GFX, DITSEngine& _DITS) 
  : UIBase(_GFX), pDits(&_DITS),
    //UIFLabel(EUID, byte _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, const __FlashStringHelper* _Label)
    //UICharVal(EUID, byte _SelectNum, bool _Settable, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, int _charSize, char* _Valchar = nullptr)
    //UIByteVal(EUID, byte _SelectNum, bool _Settable, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, byte _ValBase = HEX, byte _Vmin = 0x00, byte _Vmax = 0xFF, byte _Value = 0x00)
    //UIEnumVal(EUID, byte _SelectNum, bool _Settable, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, int _EVal = 0)
    Title   (0x01FF,1,      pGFX,TFT_COL_X(0),TFT_ROW_Y(0),F("Edit Tode")),
    LName   (0x02FF,0,      pGFX,TFT_COL_X(0),TFT_ROW_Y(1),F("Name:")),
    VName   (0x03FF,2,true, pGFX,TFT_COL_X(5),TFT_ROW_Y(1),11,ptest),
    LSecNet (0x04FF,0,      pGFX,TFT_COL_X(0),TFT_ROW_Y(2),F("SecNet:")),
    VSecNet (0x05FF,3,true, pGFX,TFT_COL_X(7),TFT_ROW_Y(2),HEX),
    LRadio  (0x06FF,0,      pGFX,TFT_COL_X(0),TFT_ROW_Y(3),F("Radio:")),
    VRadio  (0x07FF,0,false,pGFX,TFT_COL_X(6),TFT_ROW_Y(3)),
    LNetID  (0x08FF,0,      pGFX,TFT_COL_X(0),TFT_ROW_Y(4),F("NetID:")),
    VNetID  (0x08FF,4,true, pGFX,TFT_COL_X(6),TFT_ROW_Y(4)),
    LRFAddr (0x09FF,0,      pGFX,TFT_COL_X(0),TFT_ROW_Y(5),F("RFAddr:")),
    VRFAddr (0x0AFF,5,true, pGFX,TFT_COL_X(7),TFT_ROW_Y(5))
    {
      LastSelectNum = 5;
      DUID = UIEDITTODE;
      VRadio.AddEnum(0,F("NOTGOT"));
      VRadio.AddEnum(1,F("UNKNOWN"));
      VRadio.AddEnum(2,F("E22400T30D"));
      VRadio.AddEnum(3,F("E22400T33D"));
      VRadio.AddEnum(4,F("E22900T30D"));
      VRadio.AddEnum(5,F("E32433T20D"));
      VRadio.AddEnum(6,F("E32433T30D"));
      VRadio.AddEnum(7,F("E220400T30D"));
    }

    void DisplayScreen() override {
      if (DisplayedOnce) return;
      DisplayedOnce = true;
      pSelected = &Title; pSelected->Selected(true);
      pGFX->fillScreen(UI_FILLSCR_COLOR);
      pGFX->drawFastHLine(0, FONT_Y+2, TFT_WIDTH, UI_STDLINE_COLOR);
      for (int i=0; i<ArraySize(Elmt); i++) {
        Elmt[i]->Display();
      }
    }

    bool gotValue(uint16_t _DUID, uint16_t _EUID, int _Value) override {
      if (_DUID!=DUID) return false;
      for (int i=0; i<ArraySize(Elmt); i++) {
        if(Elmt[i]->EUID==_EUID) {
          Elmt[i]->Value(_Value); return true;
        }
      }
    }

    uint16_t ButtonPress(int _Button) override {
      DBUIAENTER((_Button),("DitsUI::ButtonPress\n"))
      if(_Button==NAVKEYLEFT) {
        if(pEdit!=nullptr) { return pEdit->EditButtonPress(_Button); }
        return UINODE;
      }
      if(_Button==NAVKEYRIGHT) {
        if(pEdit!=nullptr) {return pEdit->EditButtonPress(_Button);}
        return UINODE;
      }
      if(_Button==NAVKEYUP) {
        if(pEdit!=nullptr) {return pEdit->EditButtonPress(_Button);} // Edit Routing
        for (int s=pSelected->SelectNum - 1; s>0; s--) {
          for (int i=0; i<ArraySize(Elmt); i++) {
            if(Elmt[i]->SelectNum==s) {
              pSelected->Selected(false);
              pSelected = Elmt[i];
              pSelected->Selected(true);
              return UINOCHANGE;
            }
          }
        } 
      }
      if(_Button==NAVKEYDOWN) {
        if(pEdit!=nullptr) {return pEdit->EditButtonPress(_Button);}  // Edit Routing
        for (int s=pSelected->SelectNum + 1; s<=LastSelectNum; s++) {
          for (int i=0; i<ArraySize(Elmt); i++) {
            if(Elmt[i]->SelectNum==s) {
              pSelected->Selected(false);
              pSelected = Elmt[i];
              pSelected->Selected(true);
              return UINOCHANGE;
            }
          }
        }    
      }
      if(_Button==NAVKEYGET) {
        if(pEdit!=nullptr) {return pEdit->EditButtonPress(_Button);}  // Edit Routing
        //if(SelectedElmt==0) return UIEDITTODE;
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
          SetValueEUID = pEdit->EUID;
          SetValue = pEdit->Value();
          // Stage New Value and EUID for Parent to process
          pEdit = nullptr;
          return UISETVALUE;
        }
        return UINOCHANGE;
      }
      return UINOCHANGE;
    }

private:
    DITSEngine* pDits;
};