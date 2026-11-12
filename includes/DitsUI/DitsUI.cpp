#include "DitsUI.h"

char testname[11] = "TestName";

void DitsUI::Begin(DITSEngine& _pDits, Adafruit_GFX& _pGFX, Ebyte& _pEbyte)
{
    pDits = &_pDits;
    pGFX = &_pGFX;
    pEbyte = &_pEbyte;
    tmpDITName = new char[pDits->DITNameChar];
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
  uint16_t getWH = WHNONE;
  
  if(OnScreen->NSID == NSEDITTODE) {
    if(_Set&&OnScreen->SetValueWH==WHNODENAME)  { pDits->UpdateThisNodeName(OnScreen->chSetValue); }
    if(getWH==WHNODENAME||getWH==WHNONE) {
      DITSEngine::NodeDIT node = pDits->Node(0); node.NGetName(tmpDITName);
      DBAINFO((tmpDITName),("DitsUI::SGValues (tmpDITName)"))
      OnScreen->gotValue(NSEDITTODE, WHNODENAME, tmpDITName);
    }

    if(_Set&&OnScreen->SetValueWH==WHSECNET)  { pDits->SecNetCode(OnScreen->SetValue); getWH=WHSECNET; }
    if(getWH==WHSECNET||getWH==WHNONE)      { OnScreen->gotValue(NSEDITTODE, WHSECNET, pDits->SecNetCode()); }
    
    if(getWH==WHRADIOID||getWH==WHNONE)      { OnScreen->gotValue(NSEDITTODE, WHRADIOID, pEbyte->RadioID()); }
    
    if(_Set&&OnScreen->SetValueWH==WHNETID)  { pEbyte->NetID(OnScreen->SetValue); getWH=WHNETID; }
    if(getWH==WHNETID||getWH==WHNONE)      { OnScreen->gotValue(NSEDITTODE, WHNETID, pEbyte->NetID()); }
    
    if(_Set&&OnScreen->SetValueWH==WHRFADDR)  { pEbyte->Address(OnScreen->SetValue); getWH=WHRFADDR; }
    if(getWH==WHRFADDR||getWH==WHNONE)      { OnScreen->gotValue(NSEDITTODE, WHRFADDR, pEbyte->Address()); }

    OnScreen->SetValueWH = WHNONE;
    OnScreen->SetValue = 0;
  }
}

void DitsUI::ButtonPress(int _Button) {
  DBUIAENTER((_Button),("DitsUI::ButtonPress\n"))
  if (OnScreen == nullptr) return;
  uint16_t BPReply = OnScreen->ButtonPress(_Button);
  if(BPReply==NCSETVALUE) {
    SGValues(true);
  }
  if(BPReply==NSNODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if(node.NodeIdx()!=BNONE) {
      delete OnScreen;
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->DisplayScreen();
    }
  }
  if(BPReply==NCNEXTNODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if (node.Next()) {
      delete OnScreen;
      NavNodeIdx = node.NodeIdx();
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->DisplayScreen();
    }
  }
  if(BPReply==NCPREVNODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if (node.Prev()) {
      delete OnScreen;
      NavNodeIdx = node.NodeIdx();
      OnScreen = new UINode(*pGFX,*pDits,NavNodeIdx);
      OnScreen->DisplayScreen();
    }
  }
  if(BPReply==NSSETUP) {
    delete OnScreen;
    OnScreen = new UISetup(*pGFX);
    OnScreen->DisplayScreen();
  }
  if(BPReply==NSEDITTODE) {
    delete OnScreen;
    OnScreen = new UIEditTode(*pGFX);
    OnScreen->DisplayScreen();
    SGValues(); // Load the values
  }
}
