///@file    DitsUI.h
///@brief   Example DitsUI class for a node.

#include <rf_dits.h>

class DITSUI
{
public:
    void Begin(DITSEngine& dits);
    void Update();

private:
    DITSEngine* _dits = nullptr;
};

