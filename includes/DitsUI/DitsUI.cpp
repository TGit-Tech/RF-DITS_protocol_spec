#include "DitsUI.h"

char* tmpname; // General use array for Names

void DitsUI::DumpRadio1DITS() {
  Serial.println(F("\nnvvvvvvvvvvvvvvvvvvvvvvvvvv RADIO1 DITS vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
  // Step 1: Iterate through Nodes
  DITSEngine::NodeDIT node = pDits->Node();
  while (node.Next()) {
    node.NGetName(tmpname);
    Serial.print(F("NODE ["));Serial.print(node.NodeIdx());Serial.print(F("] "));
    Serial.print(tmpname);Serial.print(F(" (RF: 0x"));Serial.print(node.NRFAddr(), HEX);
    Serial.print(F(") (NDITVer: "));Serial.print(node.NDITVer());Serial.println(")");
   // Step 2: Iterate through ALL devices to find children of this node
    DITSEngine::DeviceDIT dev = pDits->Device();
    while (dev.Next(node.NodeIdx())) {    // List Devices matching node.
      dev.DGetName(tmpname);
      Serial.print(F("  |-- DevUID: "));Serial.print(dev.DevUID());
      Serial.print(F(" | Type: "));Serial.print(dev.DevType(), HEX);
      Serial.print(F(" | Attr: 0x"));Serial.print(dev.DevAttr(), HEX);
      Serial.print(F(" | Name: "));Serial.println(tmpname);
    }
  }
  Serial.println(F("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"));
}

void DitsUI::Begin(DITSEngine& _pDits, Adafruit_GFX& _pGFX)
{
    pDits = &_pDits;
    pGFX = &_pGFX;
    pinMode(DISPLIGHTPIN, OUTPUT);
    digitalWrite(DISPLIGHTPIN, DISPLIGHTACTIVE);
    
    pGFX->setRotation(2);
    pGFX->fillScreen(ST77XX_BLACK);
    pGFX->setTextColor(ST77XX_WHITE);
    pGFX->setFont(&OneSlot9pt7b);
    tmpname = new char[pDits->DITNameChar];
    Serial.print("pDits->DITNameChar=");Serial.println(pDits->DITNameChar);
}

void DitsUI::DisplayNode() {
  pGFX->drawFastHLine(0, FONT_Y+2, TFT_WIDTH, 0x03E0);
  pGFX->drawFastVLine(TFT_VAL_X, FONT_Y+2, TFT_HEIGHT - FONT_Y, 0x03E0); // Value Divider
/*
  pGFX->setCursor(0, TFT_ROW_Y(0));  pGFX->print("TodeName0.");
  pGFX->setCursor(0, TFT_ROW_Y(1));  pGFX->print("Device1");
  pGFX->setCursor(0, TFT_ROW_Y(2));  pGFX->print("Device2");
  pGFX->setCursor(0, TFT_ROW_Y(3));  pGFX->print("Device3");
  pGFX->setCursor(0, TFT_ROW_Y(4));  pGFX->print("Device4");
  pGFX->setCursor(0, TFT_ROW_Y(5));  pGFX->print("Device5");
  pGFX->setCursor(0, TFT_ROW_Y(6));  pGFX->print("Device6");
  pGFX->setCursor(0, TFT_ROW_Y(7));  pGFX->print("Device7");
  pGFX->setCursor(0, TFT_ROW_Y(8));  pGFX->print("Device8");
  pGFX->setCursor(0, TFT_ROW_Y(9));  pGFX->print("Canal Pump");
*/
  //DumpRadio1DITS();

  DITSEngine::NodeDIT node = pDits->Node();
  node.Next();
  node.NGetName(tmpname);
  pGFX->setCursor(0, TFT_ROW_Y(0));  pGFX->print(tmpname);
  pGFX->setCursor(0, TFT_ROW_Y(1));  pGFX->print(node.NRFAddr());
  
  DITSEngine::DeviceDIT dev = pDits->Device();int line=1;
  while (dev.Next(node.NodeIdx())) {
    dev.DGetName(tmpname);
    pGFX->setCursor(0, TFT_ROW_Y(line++));pGFX->print(tmpname);
  }

}