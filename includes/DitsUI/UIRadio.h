#pragma once
#include "UIBase.h"
#include <RadioI.h>

class UIRadio : public UIBase {
public:
    UIRadio(Adafruit_GFX& _GFX) : UIBase(_GFX) {}

    void DisplayScreen() override {}
    uint16_t ButtonPress(int _Button) override { return UINOCHANGE;}
    bool gotValue(uint16_t _DUID, uint16_t _EUID, int _Value) {}

private:
    //RadioComms* pRadio;
};