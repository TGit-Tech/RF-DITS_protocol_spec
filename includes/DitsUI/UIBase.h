#pragma once

/*
TODO:  WHandleUID and NavTo need uint32.
We'll use the HighByte for Static Page layouts and.
lowByte for Node/Devices with sub-screen 1/255 and sub-widgets 1/255
*/
#include "UIConfig.h"
#include <Adafruit_ST7735.h>
#include <avr/pgmspace.h>

#define TFT_ROWS        (TFT_HEIGHT / FONT_Y)
#define TFT_ROW_Y(row)  (((row) + 1) * FONT_Y)  // Position by bottom-left so row 0; actually bottom row 1.
#define TFT_COL_X(col)  (((col) + 0) * FONT_X)
#define CHARSPERROW     (TFT_WIDTH / FONT_X)

// UINum DataTypes
#define VDTBYTE       0
#define VDTUINT       1
#define VDTSINT       2
#define VDTENUM       3

#define MAXWIDGETSPERSCR 30
#define MAXWIDGETENUMS 10

template<typename T, size_t N>
constexpr size_t ArraySize(T (&)[N]) { return N; }

static const uint16_t MultiplierDEC[] = {1, 10, 100, 1000};
static const uint16_t MultiplierHEX[] = {1, 16, 256, 4096};
//--------------------------------------------------------------------------------------------------------------------------------
class UIWidget {
public:
  UIWidget(byte _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, bool _Settable, uint16_t _WHandleUID, uint16_t _NavTo)
  : SelectNum(_SelectNum), GFX(_GFX), PosX(_PosX), PosY(_PosY), Settable(_Settable), WHandleUID(_WHandleUID), NavTo(_NavTo)  {}
  virtual ~UIWidget() = default;
    
  uint8_t SelectNum;
  uint16_t WHandleUID = 0xFFFF;
  uint16_t NavTo = NSNOCHANGE;

  virtual void Display() {  }
  // Sets
  virtual bool Value(int _Value) { return false; }
  virtual bool chValue(char* _Value) { return false; }        // Read/write
  virtual bool chValue(const char* _Value) { return false; } // Read only
  // Gets
  virtual int Value() { return 0; }
  virtual char* chValue() { return nullptr; }
  virtual bool AddEnum(int _Value, const __FlashStringHelper* _Label) { return false; }
  
  virtual uint16_t EditButtonPress(int _Button) { }

  void Selected(bool _Selected) {
    if (_Selected == bSelected) return;
    bSelected = _Selected;
    Display();
  }
  
  bool Selected() { return bSelected; }
  bool SetEditMode(bool _EditMode) {
    if(_EditMode && Settable && bSelected) { EditMode = true; return true; }
    EditMode = false; return false;
  }
  
protected:
  Adafruit_GFX* GFX = nullptr;
  uint8_t PosX, PosY;
  bool Settable = false;
  bool bSelected = false;
  byte EditPos=0;
  bool EditMode = false;
};
//--------------------------------------------------------------------------------------------------------------------------------
class WFLabel : public UIWidget {
public:
  WFLabel(uint8_t _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, const __FlashStringHelper* _Label, uint16_t _WHandleUID = WHNONE, uint16_t _NavTo = NSNOCHANGE )
  : UIWidget(_SelectNum, _GFX, _PosX, _PosY, false, _WHandleUID, _NavTo), Label(_Label) {  }

  void Display() override {
    GFX->setCursor(PosX, PosY);
    if (bSelected) {GFX->setTextColor(UI_SELCHAR_COLOR);} else {GFX->setTextColor(UI_STDCHAR_COLOR);}
    if (Label!=nullptr) GFX->print(Label);
  }

private:
  const __FlashStringHelper* Label;

};
//--------------------------------------------------------------------------------------------------------------------------------
class WChLabel : public UIWidget {
public:
  WChLabel(uint8_t _SelectNum, Adafruit_GFX* _GFX,
           uint8_t _PosX, uint8_t _PosY, int _charSize,
           const char* _Label,
           uint16_t _WHandleUID = WHNONE,
           uint16_t _NavTo = NSNOCHANGE)
  : UIWidget(_SelectNum, _GFX, _PosX, _PosY, false,
             _WHandleUID, _NavTo),
    charSize(_charSize) {

    Label = new char[charSize + 1];

    if(Label != nullptr && _Label != nullptr) {
      strncpy(Label, _Label, charSize);
      Label[charSize] = '\0';
    }
  }

  ~WChLabel() override {
    delete[] Label;
  }

  void Display() override {
    GFX->setCursor(PosX, PosY);

    if(bSelected) {
      GFX->setTextColor(UI_SELCHAR_COLOR);
    } else {
      GFX->setTextColor(UI_STDCHAR_COLOR);
    }

    if(Label != nullptr) GFX->print(Label);
  }

private:
  char* Label = nullptr;
  int charSize;
};
//--------------------------------------------------------------------------------------------------------------------------------
class WCharVal : public UIWidget {
  public:
    WCharVal(uint8_t _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, int _charSize, bool _Settable = false, uint16_t _WHandleUID = WHNONE, uint16_t _NavTo = NSNOCHANGE)
    : UIWidget(_SelectNum, _GFX, _PosX, _PosY, _Settable, _WHandleUID, _NavTo), charSize(_charSize) {}

  void Display() override {
    GFX->fillRect(PosX, PosY-FONT_Y+4, FONT_X*charSize, FONT_Y-2, (EditMode) ? UIKB_NAMEEDIT_BGCOLOR : UI_FILLSCR_COLOR );
    GFX->setCursor(PosX, PosY);
    if(EditMode) {
      GFX->fillRect(PosX, PosY-FONT_Y+4, FONT_X*charSize, FONT_Y-2, UIKB_NAMEEDIT_BGCOLOR);
      GFX->fillRect(PosX+(EditPos*FONT_X),PosY-FONT_Y+4, FONT_X, FONT_Y-2, ST77XX_YELLOW); // Highlight Edit Pos
      GFX->setTextColor(ST77XX_BLACK);
    } else if (bSelected) {
      GFX->setTextColor(UI_SELCHAR_COLOR);
    } else {GFX->setTextColor(UI_STDCHAR_COLOR);}
    if (Valchar!=nullptr) {GFX->print(Valchar);} else {GFX->print(F("nullptr"));}
  }
  
  bool chValue(char* _Value) override { Valchar = _Value; Display(); return true; }
  bool chValue(const char* _Value) override { Valchar = _Value; Display(); return true; }
  char* chValue() override { return Valchar; }

  uint16_t EditButtonPress(int _Button) override { 
    if(_Button==NAVKEYLEFT && Valchar!=nullptr) { if(EditPos>0) { EditPos--; Display(); } }
    if(_Button==NAVKEYRIGHT && Valchar!=nullptr) {
      if(EditPos < charSize - 1) {
        EditPos++;
        Display();
      }
    }
    if(_Button==NAVKEYUP && Valchar!=nullptr) { 
      Valchar[EditPos] = (Valchar[EditPos] & 0xE0) | ((Valchar[EditPos] - 1) & 0x1F);
      if((Valchar[EditPos] & 0x1F) == 31) Valchar[EditPos] -= 5;
      Display();
    }
    if(_Button==NAVKEYDOWN && Valchar!=nullptr) {
      Valchar[EditPos] = (Valchar[EditPos] & 0xE0) | ((Valchar[EditPos] + 1) & 0x1F);
      if((Valchar[EditPos] & 0x1F) == 27) Valchar[EditPos] -= 27;
      Display();
    }
    if(_Button==NAVKEYGET && Valchar!=nullptr) { 
      if(Valchar!=nullptr) { Valchar[EditPos] += (Valchar[EditPos] < 96) ? 32 : -64; Display(); }
    }
    return NSNOCHANGE;
  }
  protected:
    char* Valchar = nullptr;
    int charSize;
};
//--------------------------------------------------------------------------------------------------------------------------------
class WNumVal : public UIWidget {
  public:
    uint16_t Vmin =0, Vmax =0;
    //UIWidget(byte _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, bool _Settable, uint16_t _WHandleUID)
    // BYTE (8-bit unsigned)
    WNumVal(uint8_t _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY,
      uint8_t _VDataType, uint8_t _VBase = HEX, bool _Settable = false, uint16_t _WHandleUID = WHNONE, uint16_t _NavTo = NSNOCHANGE)
      : UIWidget(_SelectNum, _GFX, _PosX, _PosY, _Settable, _WHandleUID, _NavTo), VDataType(_VDataType), VBase(_VBase)
    {
      if(VDataType==VDTBYTE) { 
        Vmin=0x0000; Vmax=0x00FF; 
        if(VBase==HEX) maxDigits = 2;
        if(VBase==DEC) maxDigits = 3;
      } else if(VDataType==VDTUINT) {
        Vmin=0x0000; Vmax=0xFFFF;
        if(VBase==HEX) maxDigits = 4;
        if(VBase==DEC) maxDigits = 5;
      } else if(VDataType==VDTSINT) {
        Vmin=(uint16_t)-32768; Vmax=32767;
        if(VBase==HEX) maxDigits = 4;
        if(VBase==DEC) maxDigits = 6;
      }
    }

    void Display() override {
      GFX->fillRect(PosX, PosY-FONT_Y+4, FONT_X*maxDigits, FONT_Y-2, (EditMode) ? UIKB_NAMEEDIT_BGCOLOR : UI_FILLSCR_COLOR);
      GFX->setCursor(PosX, PosY);

      if(EditMode) {
        GFX->fillRect(PosX+((maxDigits-1-EditPos)*FONT_X), (PosY-FONT_Y)+4, FONT_X, FONT_Y-2, ST77XX_YELLOW);
        GFX->setTextColor(ST77XX_BLACK); }
      else if(bSelected) { GFX->setTextColor(UI_SELCHAR_COLOR); }
      else { GFX->setTextColor(UI_STDCHAR_COLOR); }

      if(VDataType==VDTSINT && VBase==DEC) {
        GFX->print((int16_t)uiValue, DEC);
      } else {
        if(VDataType==VDTBYTE) {
          if((uiValue & 0xF0)==0) GFX->print('0');
        } else {
          if((uiValue & 0xF000)==0) GFX->print('0');
          if((uiValue & 0xFF00)==0) GFX->print('0');
          if((uiValue & 0xFFF0)==0) GFX->print('0');
        }
        GFX->print(uiValue, VBase);
      }
    }

    bool Value(int _Value) override {
      uiValue = (uint16_t)_Value;
      if(VDataType==VDTBYTE) uiValue &= 0x00FF;
      Display();
      return true;
    }

    int Value() override { return (int16_t)uiValue; }

    uint16_t EditButtonPress(int _Button) override {
      if(_Button==NAVKEYLEFT && EditPos<(maxDigits-1)) { EditPos++; Display(); }
      if(_Button==NAVKEYRIGHT && EditPos>0) { EditPos--; Display(); }

      if(_Button==NAVKEYUP) {
        uint16_t Step = (VBase==HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos];
        if(((uiValue/Step)%VBase)==(VBase-1)) {
          uiValue -= (uint16_t)((VBase-1)*Step);
        } else {
          uiValue += Step;
        }
        if(VDataType==VDTBYTE) uiValue &= 0x00FF;
        Display();
      }

      if(_Button==NAVKEYDOWN) {
        uint16_t Step = (VBase==HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos];
        if(((uiValue/Step)%VBase)==0) {
          uiValue += (uint16_t)((VBase-1)*Step);
        } else {
          uiValue -= Step;
        }
        if(VDataType==VDTBYTE) uiValue &= 0x00FF;
        Display();
      }
      if(_Button==NAVKEYGET) { }
      return NSNOCHANGE;
    }
  private:
    uint16_t uiValue = 0;
    byte VBase;
    byte VDataType;
    byte maxDigits = 0;
};
//--------------------------------------------------------------------------------------------------------------------------------
class WEnumVal : public UIWidget {
public:
  WEnumVal(uint8_t _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, bool _Settable = false, uint16_t _WHandleUID = WHNONE, uint16_t _NavTo = NSNOCHANGE)
  : UIWidget(_SelectNum, _GFX, _PosX, _PosY, _Settable, _WHandleUID, _NavTo) {}

  void Display() override {
    GFX->setCursor(PosX, PosY);
    if (bSelected) {GFX->setTextColor(UI_SELCHAR_COLOR);} else {GFX->setTextColor(UI_STDCHAR_COLOR);}
    bool bfound = false;
    for(int i=0;i<EnumIdx;i++) {                   // Print Enum
      if(EnumVal[i]==iEVal) {
        bfound = true;
        if (EnumLbls[i]!=nullptr) { GFX->print(EnumLbls[i]); }
        else { GFX->print("nullptr");}
        break;
      }
    }
    if(!bfound) { GFX->print("noenum"); }
  }

  bool AddEnum(int _iVal, const __FlashStringHelper* _Label ) override {
    if(EnumIdx>=MAXWIDGETENUMS) { return false; }
    if(_Label==nullptr) return false;
    byte Len = strlen_P((PGM_P)_Label);
    if(Len > MaxChars) MaxChars = Len;
    EnumVal[EnumIdx] = _iVal;
    EnumLbls[EnumIdx] = _Label;
    EnumIdx++;
    return true;
  }
  bool Value(int _Value) override { 
    iEVal = _Value;
    GFX->setCursor(PosX, PosY);
    //GFX->fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
    GFX->fillRect(PosX, PosY-FONT_Y, (MaxChars*FONT_X), FONT_Y, UI_FILLSCR_COLOR);
    Display();
    return true; 
  }
  int Value() override { return iEVal; }
private:
  const __FlashStringHelper* EnumLbls[MAXWIDGETENUMS];
  int EnumVal[MAXWIDGETENUMS];
  byte EnumIdx = 0;
  int iEVal = 0;
  byte MaxChars = 0;
};
//---------------------------------------------------------------------------------------------------------------------------------
class UIScreen {
public:
    UIScreen(Adafruit_GFX& _GFX, uint16_t _NScrIdx) : pGFX(&_GFX), NScrIdx(_NScrIdx) {}
    ~UIScreen() { for (uint8_t i = 0; i < WIdx; i++) { delete Widget[i]; Widget[i] = nullptr; } }

    uint16_t NScrIdx = 0xFFFF;
    uint16_t SetValueWH = 0xFFFF;
    int SetValue = 0;
    char* chSetValue = nullptr;

    void DisplayScreen() {
      if (DisplayedOnce) return;
      DisplayedOnce = true;
      bool OnceSelected = false;
      pGFX->fillScreen(UI_FILLSCR_COLOR);
      pGFX->drawFastHLine(0, FONT_Y+2, TFT_WIDTH, UI_STDLINE_COLOR);
      for (int i=0; i<WIdx; i++) {
        if(Widget[i] == nullptr) continue;
        if(!OnceSelected && Widget[i]->SelectNum != 0) { pSelected = Widget[i]; pSelected->Selected(true); OnceSelected = true; }
        Widget[i]->Display();
      }
    }

    uint16_t ButtonPress(int _Button) {
      DBUIAENTER((_Button),("DitsUI::ButtonPress\n"))
      if(pSelected==nullptr) return NSNOCHANGE;
      
      if(_Button==NAVKEYLEFT) {
        if(pEdit!=nullptr) { return pEdit->EditButtonPress(_Button); }
        return NSSETUP;
      }
      if(_Button==NAVKEYRIGHT) {
        if(pEdit!=nullptr) {return pEdit->EditButtonPress(_Button);}
        return NSNOCHANGE;
      }
      if(_Button==NAVKEYUP) {
        if(pEdit!=nullptr) {return pEdit->EditButtonPress(_Button);} // Edit Routing
        for (int s=pSelected->SelectNum - 1; s>0; s--) {
          for (int i=0; i<WIdx; i++) {
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
          for (int i=0; i<WIdx; i++) {
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
          SetValueWH = pEdit->WHandleUID;
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

    bool gotValue(uint16_t _NScrIdx, uint16_t _WHandleUID, int _Value) {
      if (_NScrIdx!=NScrIdx) return false;
      for (int i=0; i<WIdx; i++) {
        if(Widget[i]->WHandleUID==_WHandleUID) {
          Widget[i]->Value(_Value); return true;
        }
      }
      return false;
    }
    bool gotValue(uint16_t _NScrIdx, uint16_t _WHandleUID, char* _Value) {
      if (_NScrIdx!=NScrIdx) return false;
      for (int i=0; i<WIdx; i++) {
        if(Widget[i]->WHandleUID==_WHandleUID) {
          Widget[i]->chValue(_Value); return true;
        }
      } 
      return false; 
    }
    bool gotValue(uint16_t _NScrIdx, uint16_t _WHandleUID, const char* _Value) {
      if (_NScrIdx!=NScrIdx) return false;
      for (int i=0; i<WIdx; i++) {
        if(Widget[i]->WHandleUID==_WHandleUID) {
          Widget[i]->chValue(_Value); return true;
        }
      } 
      return false;      
    }
    
    bool NewWNumVal(uint8_t _SelectNum, uint8_t _PosX, uint8_t _PosY,
                    uint8_t _VDataType, uint8_t _VBase = HEX, bool _Settable = false,
                    uint16_t _WHandleUID = WHNONE, uint16_t _NavTo = NSNOCHANGE) {

      if(WIdx>=MAXWIDGETSPERSCR) return false;
      Widget[WIdx] = new WNumVal(_SelectNum,pGFX,_PosX,_PosY,_VDataType,_VBase,_Settable,_WHandleUID,_NavTo);
      if(_SelectNum>LastSelectNum) LastSelectNum=_SelectNum;
      WIdx++; return true;
    }
    bool NewWCharVal(uint8_t _SelectNum, uint8_t _PosX, uint8_t _PosY,
                     int _charSize, bool _Settable = false,
                     uint16_t _WHandleUID = WHNONE, uint16_t _NavTo = NSNOCHANGE) {

      if(WIdx>=MAXWIDGETSPERSCR) return false;
      Widget[WIdx] = new WCharVal(_SelectNum, pGFX,_PosX,_PosY,_charSize,_Settable,_WHandleUID,_NavTo);
      if(_SelectNum>LastSelectNum) LastSelectNum=_SelectNum;
      WIdx++; return true;
    }
    bool NewWFLabel(uint8_t _SelectNum, uint8_t _PosX, uint8_t _PosY,
                    const __FlashStringHelper* _Label,
                    uint16_t _WHandleUID = WHNONE, uint16_t _NavTo = NSNOCHANGE) {

      if(WIdx>=MAXWIDGETSPERSCR) return false;
      Widget[WIdx] = new WFLabel(_SelectNum,pGFX,_PosX,_PosY,_Label,_WHandleUID,_NavTo);
      if(_SelectNum>LastSelectNum) LastSelectNum=_SelectNum;
      WIdx++; return true;
    }
    bool NewWChLabel(uint8_t _SelectNum, uint8_t _PosX, uint8_t _PosY,
                     int _charSize, char* _Label, 
                     uint16_t _NavTo = NSNOCHANGE) {

      if(WIdx>=MAXWIDGETSPERSCR) return false;
      Widget[WIdx] = new WChLabel(_SelectNum,pGFX,_PosX,_PosY,_charSize,_Label,_NavTo);
      if(_SelectNum>LastSelectNum) LastSelectNum=_SelectNum;
      WIdx++; return true;
    }
    
    bool NewWEnumVal(uint8_t _SelectNum, uint8_t _PosX, uint8_t _PosY,
                     bool _Settable = false, 
                     uint16_t _WHandleUID = WHNONE, uint16_t _NavTo = NSNOCHANGE) {

      if(WIdx>=MAXWIDGETSPERSCR) return false;
      Widget[WIdx] = new WEnumVal(_SelectNum,pGFX,_PosX,_PosY,_Settable,_WHandleUID,_NavTo);
      if(_SelectNum>LastSelectNum) LastSelectNum=_SelectNum;
      WIdx++; return true;
    }
    bool AddEnum(uint16_t _WHandleUID, int _iVal, const __FlashStringHelper* _Label) {
      for(byte i = 0; i < WIdx; i++) {
        if(Widget[i]->WHandleUID == _WHandleUID) { return Widget[i]->AddEnum(_iVal,_Label); }
      }
      return false;
    }
protected:
    Adafruit_GFX* pGFX = nullptr;
    bool DisplayedOnce = false;
    int LastSelectNum = 0;
    int SelectedRow = 0;
    UIWidget* pSelected = nullptr; 
    UIWidget* pEdit = nullptr;
    UIWidget* Widget[MAXWIDGETSPERSCR] = {nullptr};
    byte WIdx = 0;
};