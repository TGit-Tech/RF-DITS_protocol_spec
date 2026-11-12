#pragma once

#include "UIConfig.h"
#include "UIBase.h"


class UIRadio : public UIBase {
public:
    UIRadio(Adafruit_GFX& _GFX) : UIBase(_GFX) {}

    void DisplayScreen() override {}
    uint16_t ButtonPress(int _Button) override { return NSNOCHANGE;}
    bool gotValue(uint16_t _NSID, uint16_t _EUID, int _Value) {}

private:
    //RadioComms* pRadio;
};