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
      DisplayNode(node);
    }
}

void DitsUI::SGValues(bool _Set = false) {
  DBUIAENTER((_Set),("DitsUI::SGValues(Set)\n"))
  if(OnScreen==nullptr) return;
  uint16_t getWH = WHNONE;
  
  if(OnScreen->NScrIdx == NSEDITTODE) {
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

void DitsUI::DisplayNode(DITSEngine::NodeDIT& node) {
  delete OnScreen; OnScreen = nullptr;

  int row = 0;
  OnScreen = new UIScreen(*pGFX,NSNODE);
  node.NGetName(tmpDITName);
  OnScreen->NewWChLabel(row+1,TFT_COL_X(5), TFT_ROW_Y(row),pDits->DITNameChar,tmpDITName);

  DITSEngine::DeviceDIT dev = pDits->Device();
  while (row < TFT_ROWS - 1 && dev.Next(node.NodeIdx())) {
    row++; // Note: WHandleUID is DevUID
    dev.DGetName(tmpDITName);
    OnScreen->NewWChLabel(row+1,TFT_COL_X(5), TFT_ROW_Y(row),pDits->DITNameChar,tmpDITName);
  }
  OnScreen->DisplayScreen();
}

void DitsUI::ButtonPress(int _Button) {
  DBUIAENTER((_Button),("DitsUI::ButtonPress\n"))
  if (OnScreen == nullptr) return;
  uint16_t BPReply = NSNOCHANGE;
  if(OnScreen->NScrIdx==NSNODE && _Button==NAVKEYSET) {BPReply=NSSETUP;}
  else { BPReply = OnScreen->ButtonPress(_Button); }
  if(BPReply==NCSETVALUE) {
    SGValues(true);
  }
  if(BPReply==NSNODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if(node.NodeIdx()!=BNONE) {
      NavNodeIdx = node.NodeIdx();
      DisplayNode(node);
    }
  }
  if(BPReply==NCNEXTNODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if (node.Next()) {
      NavNodeIdx = node.NodeIdx();
      DisplayNode(node);
    }
  }
  if(BPReply==NCPREVNODE) {
    DITSEngine::NodeDIT node = pDits->Node(NavNodeIdx);
    if (node.Prev()) {
      NavNodeIdx = node.NodeIdx();
      DisplayNode(node);
    }
  }
  if(BPReply==NSSETUP) {
    delete OnScreen;
    //OnScreen = new UISetup(*pGFX);
    OnScreen = new UIScreen(*pGFX,NSSETUP);
    OnScreen->NewWFLabel  (1,TFT_COL_X(0), TFT_ROW_Y(0), F("Setup Menu"),WHNONE,NSNODE);
    OnScreen->NewWFLabel  (2,TFT_COL_X(0), TFT_ROW_Y(1), F("Add RTode"), WHNONE,NSNOCHANGE);
    OnScreen->NewWFLabel  (3,TFT_COL_X(0), TFT_ROW_Y(2), F("Del RTode"), WHNONE,NSNOCHANGE);
    OnScreen->NewWFLabel  (4,TFT_COL_X(0), TFT_ROW_Y(3), F("Edit Tode"), WHNONE,NSEDITTODE);
    OnScreen->NewWFLabel  (5,TFT_COL_X(0), TFT_ROW_Y(4), F("Add Dev"),   WHNONE,NSNOCHANGE);
    OnScreen->NewWFLabel  (6,TFT_COL_X(0), TFT_ROW_Y(5), F("Del Dev"),   WHNONE,NSNOCHANGE);
    OnScreen->NewWFLabel  (7,TFT_COL_X(0), TFT_ROW_Y(6), F("Edit Dev"),  WHNONE,NSNOCHANGE);
    OnScreen->NewWFLabel  (8,TFT_COL_X(0), TFT_ROW_Y(7), F("System"),    WHNONE,NSNOCHANGE);

    OnScreen->DisplayScreen();
  }
  if(BPReply==NSEDITTODE) {
    delete OnScreen;
    //OnScreen = new UIEditTode(*pGFX);
    OnScreen = new UIScreen(*pGFX,NSEDITTODE);
    OnScreen->NewWFLabel  (1,TFT_COL_X(0), TFT_ROW_Y(0), F("Edit Tode"));
    OnScreen->NewWFLabel  (0,TFT_COL_X(0), TFT_ROW_Y(1), F("Name:"));
    OnScreen->NewWCharVal (2,TFT_COL_X(5), TFT_ROW_Y(1), 11, true, WHNODENAME);
    OnScreen->NewWFLabel  (0,TFT_COL_X(0), TFT_ROW_Y(2), F("SecNet:"));
    OnScreen->NewWNumVal  (3,TFT_COL_X(7), TFT_ROW_Y(2), VDTBYTE, HEX, true, WHSECNET);
    OnScreen->NewWFLabel  (0,TFT_COL_X(0), TFT_ROW_Y(3), F("Radio:"));
    OnScreen->NewWEnumVal (0,TFT_COL_X(6), TFT_ROW_Y(3), false, WHRADIOID);
    OnScreen->NewWFLabel  (0,TFT_COL_X(0), TFT_ROW_Y(4), F("NetID:"));
    OnScreen->NewWNumVal  (4,TFT_COL_X(6), TFT_ROW_Y(4), VDTUINT, HEX, true, WHNETID);
    OnScreen->NewWFLabel  (0,TFT_COL_X(0), TFT_ROW_Y(5), F("RFAddr:"));
    OnScreen->NewWNumVal  (5,TFT_COL_X(7), TFT_ROW_Y(5), VDTUINT, HEX, true, WHRFADDR);

    OnScreen->AddEnum(WHRADIOID,0,F("NOTGOT"));
    OnScreen->AddEnum(WHRADIOID,1,F("UNKNOWN"));
    OnScreen->AddEnum(WHRADIOID,2,F("E22400T30D"));
    OnScreen->AddEnum(WHRADIOID,3,F("E22400T33D"));
    OnScreen->AddEnum(WHRADIOID,4,F("E22900T30D"));
    OnScreen->AddEnum(WHRADIOID,5,F("E32433T20D"));
    OnScreen->AddEnum(WHRADIOID,6,F("E32433T30D"));
    OnScreen->AddEnum(WHRADIOID,7,F("E220400T30D"));

    OnScreen->DisplayScreen();
    SGValues(); // Load the values
  }
}
