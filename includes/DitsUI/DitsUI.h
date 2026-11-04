///@file    DitsUI.h
///@brief   Example DitsUI class for a node.

#include <RF-DITS.h>

class DITSUI
{
public:
    void Begin(RF_DITS& dits);
    void Update();

private:
    RF_DITS* _dits = nullptr;
};

