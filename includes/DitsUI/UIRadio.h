#pragma once

class UIRadio : public UIBase {
public:
    UIRadio(Adafruit_GFX& _GFX) : UIBase(_GFX) {}

    void Display() override {}
    byte ButtonPress(int _Button) override { return UINOCHANGE;}

private:
    //RadioComms* pRadio;
};