#include "DITSUI.h"

void DITSUI::Begin(DITSEngine& dits)
{
    _dits = &dits;
}

void DITSUI::Update()
{
    if (!_dits)
        return;

    // TFT/keypad interaction using *_dits
}