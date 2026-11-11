#pragma once
#include "UIBase.h"

//static const uint16_t MultiplierDEC[] = {1000, 100, 10, 1};
//static const uint16_t MultiplierHEX[] = {4096, 256, 16, 1};
static const uint16_t MultiplierDEC[] = {1, 10, 100, 1000};
static const uint16_t MultiplierHEX[] = {1, 16, 256, 4096};

class UIPerChValEdit : public UIBase {
private:
  uint8_t X, Y, base=0, nameSize=0;
  char* pName = nullptr;
  int Val=0, minVal=0, maxVal=0;
  byte EditPos=0;
public:
    UIPerChValEdit(Adafruit_GFX& _GFX, uint8_t _X, uint8_t _Y, char* _Name, uint8_t _Size) 
    : UIBase(_GFX), X(_X), Y(_Y), pName(_Name), nameSize(_Size) {
    }
    UIPerChValEdit(Adafruit_GFX& _GFX, uint8_t _X, uint8_t _Y, int _Val, int _minVal, int _maxVal, uint8_t _base = HEX) 
    : UIBase(_GFX), X(_X), Y(_Y), Val(_Val), minVal(_minVal), maxVal(_maxVal), base(_base) {
      
    }

    void DisplayScreen() override { 
      //fillRect(x, y, width, height, color);
      if(pName!=nullptr) { pGFX->fillRect(X, Y-FONT_Y+4, FONT_X*nameSize, FONT_Y-2, UIKB_NAMEEDIT_BGCOLOR); } //BG
      else               { pGFX->fillRect(X, Y-FONT_Y+4, FONT_X*4,        FONT_Y-2, UIKB_NAMEEDIT_BGCOLOR); } //4ch numeric
      if(pName!=nullptr) { pGFX->fillRect(X+(EditPos*FONT_X),   Y-FONT_Y+4, FONT_X, FONT_Y-2, ST77XX_YELLOW); } // Edit Pos
      else               { pGFX->fillRect(X+(EditPos*FONT_X)+3, Y-FONT_Y+4, FONT_X, FONT_Y-2, ST77XX_YELLOW); }
      pGFX->setCursor(X, Y);
      pGFX->setTextColor(ST77XX_BLACK);
      if(pName!=nullptr) { pGFX->print(pName); } else {
        if((Val & 0xF000) == 0) pGFX->print('0'); // Manually print leading '0's.
        if((Val & 0xFF00) == 0) pGFX->print('0');
        if((Val & 0xFFF0) == 0) pGFX->print('0'); 
        pGFX->print((uint16_t)Val,base); 
      }
    }

    uint16_t ButtonPress(int _Button) override { 
      if(_Button==NAVKEYLEFT) { 
        if(pName!=nullptr) { 
          if(EditPos>0) { EditPos--; Display(); }
        } else { 
          if(EditPos<3) { EditPos++; Display(); }
        }
      }
      if(_Button==NAVKEYRIGHT) { 
        if(pName!=nullptr) { 
          EditPos++; Display();
        } else {
          if(EditPos>0) { EditPos--; Display(); }
        }
      }
      if(_Button==NAVKEYUP) {
        if(pName!=nullptr) { 
          pName[EditPos] = (pName[EditPos] & 0xE0) | ((pName[EditPos] - 1) & 0x1F);
          if((pName[EditPos] & 0x1F) == 31) pName[EditPos] -= 5;
        } else {
          if((Val / ((base == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos])) % base == base-1) {
            Val -= (base-1) * ((base == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos]);
          } else {
            Val += (base == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos];
          }
        }
        Display();
      }
      if(_Button==NAVKEYDOWN) {
        if(pName!=nullptr)  { 
          pName[EditPos] = (pName[EditPos] & 0xE0) | ((pName[EditPos] + 1) & 0x1F);
          if((pName[EditPos] & 0x1F) == 27) pName[EditPos] -= 27;
        } else {
          if((Val / ((base == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos])) % base == 0) {
            Val += (base-1) * ((base == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos]);
          } else {
            Val -= (base == HEX) ? MultiplierHEX[EditPos] : MultiplierDEC[EditPos];
          }
        }
        Display();
      }
      if(_Button==NAVKEYGET) { 
        if(pName!=nullptr) { pName[EditPos] += (pName[EditPos] < 96) ? 32 : -64; Display(); }
      }
      return UINOCHANGE;
    }
};