#include "DitsUI.h"

char testname[11] = "TestName";

void DitsUI::Begin(DITSEngine& _pDits, Adafruit_GFX& _pGFX, Ebyte& _pEbyte)
{
    pDits = &_pDits;
    pGFX = &_pGFX;
    pEbyte = &_pEbyte;
    pinMode(DISPLIGHTPIN, OUTPUT);
    digitalWrite(DISPLIGHTPIN, DISPLIGHTACTIVE);
    
    pGFX->setRotation(2);
    pGFX->setFont(&OneSlot9pt7b);
    
    Serial.print("pDits->DITNameChar=");Serial.println(pDits->DITNameChar);

    DITSEngine::NodeDIT node = pDits->Node();
    if (node.Next()) {
      NavNodeIdx = node.NodeIdx();
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->DisplayScreen();
    }
}
void DitsUI::SGValues(bool _Set = false) {
  DBUIAENTER((_Set),("DitsUI::SGValues(Set)\n"))
  if(OnScreen==nullptr) return;
  uint16_t GetEUID = 0xFFFF;
  if(OnScreen->DUID == UIEDITTODE) {
    if(GetEUID==0x07FF||GetEUID==0xFFFF) OnScreen->gotValue(UIEDITTODE, 0x07FF, pEbyte->RadioID());
    if(_Set&&OnScreen->SetValueEUID==0x08FF) { pEbyte->NetID(OnScreen->SetValue); GetEUID=0x08FF; }
    if(GetEUID==0x08FF||GetEUID==0xFFFF) OnScreen->gotValue(UIEDITTODE, 0x08FF, pEbyte->NetID());
    if(_Set&&OnScreen->SetValueEUID==0x0AFF) { pEbyte->Address(OnScreen->SetValue); GetEUID=0x0AFF; }
    if(GetEUID==0x0AFF||GetEUID==0xFFFF) OnScreen->gotValue(UIEDITTODE, 0x0AFF, pEbyte->Address());
    OnScreen->SetValueEUID = 0xFFFF;
    OnScreen->SetValue = 0;
  }
}

void DitsUI::ButtonPress(int _Button) {
  DBUIAENTER((_Button),("DitsUI::ButtonPress\n"))
  if (OnScreen == nullptr) return;
  uint16_t BPReply = OnScreen->ButtonPress(_Button);
  if(BPReply==UISETVALUE) {
    SGValues(true);
  }
  if(BPReply==UINODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if(node.NodeIdx()!=BNONE) {
      delete OnScreen;
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->DisplayScreen();
    }
  }
  if(BPReply==UINEXTNODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if (node.Next()) {
      delete OnScreen;
      NavNodeIdx = node.NodeIdx();
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->DisplayScreen();
    }
  }
  if(BPReply==UIPREVNODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if (node.Prev()) {
      delete OnScreen;
      NavNodeIdx = node.NodeIdx();
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->DisplayScreen();
    }
  }
  if(BPReply==UISETUP) {
    delete OnScreen;
    OnScreen = new UISetup(*pGFX);
    OnScreen->DisplayScreen();
  }
  if(BPReply==UIEDITTODE) {
    delete OnScreen;
    OnScreen = new UIEditTode(*pGFX, *pDits);
    OnScreen->DisplayScreen();
    SGValues(); // Load the values
  }
}
