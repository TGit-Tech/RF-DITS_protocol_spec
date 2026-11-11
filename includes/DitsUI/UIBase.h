#pragma once
//#include <Arduino.h>
#include <Adafruit_ST7735.h>
#include <avr/pgmspace.h>

#define TFT_ROWS        (TFT_HEIGHT / FONT_Y)
#define TFT_ROW_Y(row)  (((row) + 1) * FONT_Y)  // Position by bottom-left so row 0; actually bottom row 1.
#define TFT_COL_X(col)  (((col) + 0) * FONT_X)
#define CHARSPERROW     (TFT_WIDTH / FONT_X)

#define UISETVALUE   0x00FF // Set a new value from OnScreen->SetValueEUID and OnScreen->SetValue.
#define UIGETVALUE   0x01FF // Load freshly gotten values into UI
#define UINOCHANGE   0x02FF
#define UINODE       0x03FF
#define UINEXTNODE   0x04FF
#define UIPREVNODE   0x05FF
#define UISETUP      0x06FF
#define UIEDITTODE   0x07FF
#define UIRADIO      0x08FF
#define UIADDDEV     0x09FF
#define UIDELNODE    0x0AFF
#define UIADDNODE    0x0BFF
#define UISYSTEM     0x0CFF

// Color Scheme
#define UI_FILLSCR_COLOR        ST77XX_BLACK
#define UI_SELCHAR_COLOR        ST77XX_BLUE
#define UI_STDCHAR_COLOR        ST77XX_WHITE
#define UI_STDLINE_COLOR        0x03E0

#define UIKB_NAMEEDIT_BGCOLOR   0x8410
#define UIKB_SELNAMECHAR_COLOR  0x001F
#define UIKB_STDNAMECHAR_COLOR  ST77XX_WHITE
#define UIKB_SELCHAR_COLOR      ST77XX_BLUE
#define UIKB_STDCHAR_COLOR      ST77XX_WHITE
#define GFXYELLOW 0xFFDE
 /*
 // Some ready-made 16-bit ('565') color settings:
 #define ST77XX_BLACK 0x0000
 #define ST77XX_WHITE 0xFFFF
 #define ST77XX_RED 0xF800
 #define ST77XX_GREEN 0x07E0
 #define ST77XX_BLUE 0x001F
 #define ST77XX_CYAN 0x07FF
 #define ST77XX_MAGENTA 0xF81F
 #define ST77XX_YELLOW 0xFFE0
 #define ST77XX_ORANGE 0xFC00
*/

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
#ifndef NAVKEYGET
  #define NAVKEYGET        0xF9
#endif 
#ifndef KEYPRESSMIN
  #define KEYPRESSMIN       0xF0
#endif 
///@}

template<typename T, size_t N>
constexpr size_t ArraySize(T (&)[N]) { return N; }

static const uint16_t MultiplierDEC[] = {1, 10, 100, 1000};
static const uint16_t MultiplierHEX[] = {1, 16, 256, 4096};
//--------------------------------------------------------------------------------------------------------------------------------
class UIElement {
public:
  UIElement(uint16_t _EUID, byte _SelectNum, bool _Settable, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY)
  : EUID(_EUID), SelectNum(_SelectNum), Settable(_Settable), GFX(_GFX), PosX(_PosX), PosY(_PosY) {}
  virtual ~UIElement() = default;
    
  uint8_t SelectNum;
  uint16_t EUID = 0xFFFF;

  virtual void Display() {  }
  virtual void Value(int _Value) { Display(); }
  virtual int Value() { return 0; }
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
class UIFLabel : public UIElement {
public:
  UIFLabel(uint16_t _EUID, byte _SelectNum, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, const __FlashStringHelper* _Label)
  : UIElement(_EUID, _SelectNum, false, _GFX, _PosX, _PosY), Label(_Label) {  }
    
  void Display() override {
    GFX->setCursor(PosX, PosY);
    if (bSelected) {GFX->setTextColor(UI_SELCHAR_COLOR);} else {GFX->setTextColor(UI_STDCHAR_COLOR);}
    if (Label!=nullptr) GFX->print(Label);
  }

private:
  const __FlashStringHelper* Label;

};
//--------------------------------------------------------------------------------------------------------------------------------
class UICharVal : public UIElement {
  public:
    UICharVal(uint16_t _EUID, byte _SelectNum, bool _Settable, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, int _charSize, char* _Valchar = nullptr)
    : UIElement(_EUID, _SelectNum, _Settable, _GFX, _PosX, _PosY), charSize(_charSize), Valchar(_Valchar) {}

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
    return UINOCHANGE;
  }
  private:
    char* Valchar;
    int charSize;
};
//--------------------------------------------------------------------------------------------------------------------------------
class UIByteVal : public UIElement {
  public:
    UIByteVal(uint16_t _EUID, byte _SelectNum, bool _Settable, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, byte _ValBase = HEX, byte _Vmin = 0x00, byte _Vmax = 0xFF, byte _Value = 0x00)
    : UIElement(_EUID, _SelectNum, _Settable, _GFX, _PosX, _PosY), ValBase(_ValBase), Vmin(_Vmin), Vmax(_Vmax), yValue(_Value) {
      if(ValBase==HEX) maxDigits = 2;
      if(ValBase==DEC) maxDigits = 3;
    }

    void Display() override {
      GFX->fillRect(PosX, PosY-FONT_Y+4, FONT_X*maxDigits, FONT_Y-2, (EditMode) ? UIKB_NAMEEDIT_BGCOLOR : UI_FILLSCR_COLOR ); // Looks good
      GFX->setCursor(PosX, PosY);
      if(EditMode) {
        GFX->fillRect(PosX+((maxDigits-1-EditPos)*FONT_X), (PosY-FONT_Y)+4, FONT_X, FONT_Y-2, ST77XX_YELLOW); // Yellow Digit;
        GFX->setTextColor(ST77XX_BLACK);
      } else if (bSelected) {
        GFX->setTextColor(UI_SELCHAR_COLOR);
      } else {GFX->setTextColor(UI_STDCHAR_COLOR);}
      if((yValue & 0xF0) == 0) GFX->print('0'); // Manually print leading '0's.
      GFX->print(yValue,ValBase);
    }

    void Value(int _Value) override { yValue = (byte)_Value; Display(); }
    int Value() override { return (int)yValue; }

    uint16_t EditButtonPress(int _Button) override { 
      if(_Button==NAVKEYLEFT && EditPos<(maxDigits-1)) { 
        EditPos++; Display();
      }
      if(_Button==NAVKEYRIGHT && EditPos>0) { 
        EditPos--; Display();
      }
      if(_Button==NAVKEYUP) {
        if( (yValue / ( (ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos]) ) % ValBase == ValBase-1) {
          yValue -= (ValBase-1) * ((ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos]);
        } else {
          yValue += (ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos];
        }
        Display();
      }
      if(_Button==NAVKEYDOWN) {
        if((yValue / ((ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos])) % ValBase == 0) {
          yValue += (ValBase-1) * ((ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos]);
        } else {
          yValue -= (ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos];
        }
        Display();
      }
      if(_Button==NAVKEYGET) { }
      return UINOCHANGE;
    }    
  private:
    byte yValue, Vmin, Vmax, ValBase;
    int maxDigits = 0;
};
//---------------------------------------------------------------------------------------------------------------------------------
class UIUIntVal : public UIElement {
  public:
    UIUIntVal(uint16_t _EUID, byte _SelectNum, bool _Settable, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, byte _ValBase = HEX, uint16_t _Vmin = 0x0000, uint16_t _Vmax = 0xFFFF, uint16_t _Value = 0x0000)
    : UIElement(_EUID, _SelectNum, _Settable, _GFX, _PosX, _PosY), ValBase(_ValBase), Vmin(_Vmin), Vmax(_Vmax), uiValue(_Value) {
      if(ValBase==DEC) maxDigits = 5;
      if(ValBase==HEX) maxDigits = 4;
    }
  
    void Display() override {
      GFX->fillRect(PosX, PosY-FONT_Y+4, FONT_X*maxDigits, FONT_Y-2, (EditMode) ? UIKB_NAMEEDIT_BGCOLOR : UI_FILLSCR_COLOR );
      GFX->setCursor(PosX, PosY);
      if(EditMode) {
        GFX->fillRect(PosX+((maxDigits-1-EditPos)*FONT_X), (PosY-FONT_Y)+4, FONT_X, FONT_Y-2, ST77XX_YELLOW); // Yellow Digit;
        GFX->setTextColor(ST77XX_BLACK);
      } else if (bSelected) {
        GFX->setTextColor(UI_SELCHAR_COLOR);
      } else { GFX->setTextColor(UI_STDCHAR_COLOR); }
      if((uiValue & 0xF000) == 0) GFX->print('0'); // Manually print leading '0's.
      if((uiValue & 0xFF00) == 0) GFX->print('0');
      if((uiValue & 0xFFF0) == 0) GFX->print('0');
      GFX->print(uiValue,ValBase);
    }

    void Value(int _Value) override { uiValue = (uint16_t)_Value; Display(); }
    int Value() override { return (uint16_t)uiValue; } 

    uint16_t EditButtonPress(int _Button) override { 
      if(_Button==NAVKEYLEFT && EditPos<(maxDigits-1)) { 
        EditPos++; Display();
      }
      if(_Button==NAVKEYRIGHT && EditPos>0) { 
        EditPos--; Display();
      }
      if(_Button==NAVKEYUP) {
        if( (uiValue / ( (ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos]) ) % ValBase == ValBase-1) {
          uiValue -= (ValBase-1) * ((ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos]);
        } else {
          uiValue += (ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos];
        }
        Display();
      }
      if(_Button==NAVKEYDOWN) {
        if((uiValue / ((ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos])) % ValBase == 0) {
          uiValue += (ValBase-1) * ((ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos]);
        } else {
          uiValue -= (ValBase == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos];
        }
        Display();
      }
      if(_Button==NAVKEYGET) { }
      return UINOCHANGE;
    } 
  private:
    byte ValBase;
    uint16_t uiValue, Vmin, Vmax;
    int maxDigits = 0;
};
//---------------------------------------------------------------------------------------------------------------------------------
class UISINTVal : public UIElement {
  public:
    UISINTVal(uint16_t _EUID, byte _SelectNum, bool _Settable, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, byte _ValBase, int _Vmin = -32768, int _Vmax = 32767, int _Value = 0)
    : UIElement(_EUID, _SelectNum, _Settable, _GFX, _PosX, _PosY), ValBase(_ValBase), Vmin(_Vmin), Vmax(_Vmax), iValue(_Value) {}

    void Display() override {
      GFX->setCursor(PosX, PosY);
      if (bSelected) {GFX->setTextColor(UI_SELCHAR_COLOR);} else {GFX->setTextColor(UI_STDCHAR_COLOR);}
      GFX->print(iValue,ValBase);
    }
    void Value(int _Value) override { iValue = _Value; Display(); }
    int Value() override { return iValue; } 
  private:
    byte ValBase;
    int iValue, Vmin, Vmax;
};
//---------------------------------------------------------------------------------------------------------------------------------
class UIEnumVal : public UIElement {
public:
  UIEnumVal(uint16_t _EUID, byte _SelectNum, bool _Settable, Adafruit_GFX* _GFX, uint8_t _PosX, uint8_t _PosY, int _EVal = 0)
  : UIElement(_EUID, _SelectNum, _Settable, _GFX, _PosX, _PosY), iEVal(_EVal) {}
  
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
  void Value(int _Value) override { 
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
    
    //DUID is Display UID like 'UITodeEdit' must be beyond settable Node#'s to prevent conflict.
    //EUID is Element UID which will be devUID equivalent for UINode
    //RxDataDevValue(byte _NodeIdx, byte _DevUID, int value)
    // Returns 'true' if DUID and EUID is found and value updated.
    virtual bool gotValue(uint16_t _DUID, uint16_t _EUID, int _Value) = 0;
    uint16_t DUID = 0xFFFF;
    uint16_t SetValueEUID = 0xFFFF;
    int SetValue = 0;

protected:
    Adafruit_GFX* pGFX = nullptr;
    bool DisplayedOnce = false;
    int LastSelectNum = 0;
};