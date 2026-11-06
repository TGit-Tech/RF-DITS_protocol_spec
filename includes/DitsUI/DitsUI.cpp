#include "DitsUI.h"

void DitsUI::Begin(DITSEngine& _pDits, Adafruit_GFX& _pGFX)
{
    pDits = &_pDits;
    pGFX = &_pGFX;
    pinMode(DISPLIGHTPIN, OUTPUT);
    digitalWrite(DISPLIGHTPIN, DISPLIGHTACTIVE);
    
    pGFX->setRotation(2);
    pGFX->setFont(&OneSlot9pt7b);
    
    Serial.print("pDits->DITNameChar=");Serial.println(pDits->DITNameChar);

    DITSEngine::NodeDIT node = pDits->Node();
    if (node.Next()) {
      NavNodeIdx = node.NodeIdx();
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->Display();
    }
}

void DitsUI::ButtonPress(int _Button) {
  DBUIAENTER((_Button),("DitsUI::ButtonPress\n"))
  if (OnScreen == nullptr) return;
  byte NextScreen = OnScreen->ButtonPress(_Button);
  if(NextScreen==UINODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if(node.NodeIdx()!=BNONE) {
      delete OnScreen;
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->Display();
    }
  }
  if(NextScreen==UINEXTNODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if (node.Next()) {
      delete OnScreen;
      NavNodeIdx = node.NodeIdx();
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->Display();
    }
  }
  if(NextScreen==UIPREVNODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if (node.Prev()) {
      delete OnScreen;
      NavNodeIdx = node.NodeIdx();
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->Display();
    }
  }
  if(NextScreen==UISETUP) {
    delete OnScreen;
    OnScreen = new UISetup(*pGFX);
    OnScreen->Display();
  }
}