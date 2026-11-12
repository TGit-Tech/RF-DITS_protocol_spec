#pragma once

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

template<typename T, size_t N>
constexpr size_t ArraySize(T (&)[N]) { return N; }

static const uint16_t MultiplierDEC[] = {1, 10, 100, 1000};
static const uint16_t MultiplierHEX[] = {1, 16, 256, 4096};
//--------------------------------------------------------------------------------------------------------------------------------
class UIWidget {
public:
  UIWidget(byte _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, bool _Settable, uint16_t _WHandle)
  : SelectNum(_SelectNum), GFX(_GFX), PosX(_PosX), PosY(_PosY), Settable(_Settable), WHandle(_WHandle)  {}
  virtual ~UIWidget() = default;
    
  uint8_t SelectNum;
  uint16_t WHandle = 0xFFFF;

  virtual void Display() {  }
  // Sets
  virtual bool Value(int _Value) { return false; }
  virtual bool chValue(char* _Value) { return false; }        // Read/write
  virtual bool chValue(const char* _Value) { return false; } // Read only
  // Gets
  virtual int Value() { return 0; }
  virtual char* chValue() { return nullptr; }

  virtual uint16_t EditButtonPress(int _Button) {

  }

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
  WFLabel(uint8_t _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, const __FlashStringHelper* _Label, uint16_t _WHandle = WHNONE)
  : UIWidget(_SelectNum, _GFX, _PosX, _PosY, false, _WHandle), Label(_Label) {  }

  void Display() override {
    GFX->setCursor(PosX, PosY);
    if (bSelected) {GFX->setTextColor(UI_SELCHAR_COLOR);} else {GFX->setTextColor(UI_STDCHAR_COLOR);}
    if (Label!=nullptr) GFX->print(Label);
  }

private:
  const __FlashStringHelper* Label;

};
//--------------------------------------------------------------------------------------------------------------------------------
class WCharVal : public UIWidget {
  public:
    WCharVal(uint8_t _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, int _charSize, bool _Settable = false, uint16_t _WHandle = WHNONE)
    : UIWidget(_SelectNum, _GFX, _PosX, _PosY, _Settable, _WHandle), charSize(_charSize) {}

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
    if(_Button==NAVKEYRIGHT && Valchar!=nullptr) { EditPos++; Display(); }
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
  private:
    char* Valchar;
    int charSize;
};
//--------------------------------------------------------------------------------------------------------------------------------
class WNumVal : public UIWidget {
  public:
    uint16_t Vmin =0, Vmax =0;
    //UIWidget(byte _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, bool _Settable, uint16_t _WHandle)
    // BYTE (8-bit unsigned)
    WNumVal(uint8_t _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY,
      uint8_t _VDataType, uint8_t _VBase = HEX, bool _Settable = false, uint16_t _WHandle = WHNONE)
      : UIWidget(_SelectNum, _GFX, _PosX, _PosY, _Settable, _WHandle), VDataType(_VDataType), VBase(_VBase)
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
  WEnumVal(uint8_t _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, bool _Settable = false, uint16_t _WHandle = WHNONE)
  : UIWidget(_SelectNum, _GFX, _PosX, _PosY, _Settable, _WHandle) {}

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

  void AddEnum(int _iVal, const __FlashStringHelper* _Label ) {
    if(EnumIdx>9) { return; }
    EnumVal[EnumIdx] = _iVal;
    EnumLbls[EnumIdx] = _Label;
    EnumIdx++;
  }
  bool Value(int _Value) override { 
    iEVal = _Value;
    if(MaxChars==0) {
      for (byte i = 0; i < EnumIdx; i++) {
        byte Len = strlen_P((PGM_P)EnumLbls[i]);
        if (Len > MaxChars) MaxChars = Len;
      }
    }
    GFX->setCursor(PosX, PosY);
    //GFX->fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
    GFX->fillRect(PosX, PosY-FONT_Y, (MaxChars*FONT_X), FONT_Y, UI_FILLSCR_COLOR);
    Display();
    return true; 
  }
  int Value() override { return iEVal; }
private:
  const __FlashStringHelper* EnumLbls[10];
  int EnumVal[10];
  byte EnumIdx = 0;
  int iEVal = 0;
  byte MaxChars = 0;
};
//---------------------------------------------------------------------------------------------------------------------------------
class UIBase {
public:
    UIBase(Adafruit_GFX& _GFX) : pGFX(&_GFX) {}

    virtual void DisplayScreen() = 0;
    virtual uint16_t ButtonPress(int _Button) = 0;
    
    //NSID is Display UID like 'UITodeEdit' must be beyond settable Node#'s to prevent conflict.
    //EUID is Element UID which will be devUID equivalent for UINode
    //RxDataDevValue(byte _NodeIdx, byte _DevUID, int value)
    // Returns 'true' if NSID and EUID is found and value updated.
    virtual bool gotValue(uint16_t _NSID, uint16_t _EUID, int _Value) = 0;
    virtual bool gotValue(uint16_t _NSID, uint16_t _EUID, char* _Value) { return false; }       // Read/write
    virtual bool gotValue(uint16_t _NSID, uint16_t _EUID, const char* _Value) { return false; } // Read only
    
    uint16_t NSID = 0xFFFF;
    uint16_t SetValueWH = 0xFFFF;
    int SetValue = 0;
    char* chSetValue = nullptr;

protected:
    Adafruit_GFX* pGFX = nullptr;
    bool DisplayedOnce = false;
    int LastSelectNum = 0;
};