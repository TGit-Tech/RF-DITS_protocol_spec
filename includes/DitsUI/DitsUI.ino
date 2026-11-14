#include "Config.h"
#include <rf_dits.h>
#include "DitsUI.h"
#include <Adafruit_ST7735.h>
#include "DigKeyPadBD241S.h"
#include <Ebyte.h>

//#include "C:/Users/TGIT-TECH/Documents/!GitHub/RF-DITS_protocol_spec/includes/RadioI"

/*  This is an example using *real* persistant memory
// -------------------------------------------------------------------------------------------------
// EEPROM instance of DITSEngine Call-back Implementation via 'overloading'
// -------------------------------------------------------------------------------------------------
class EEmgrDIT : public DITSEngine {
  public:
    //DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes);
    EEmgrDIT() : DITSEngine(0,140,10) {} // 0x0800 = 2,048-bytes
  protected:
    virtual int RxReqDeviceValue(byte _DevUID) override { return 42; }
    virtual void RxReqDeviceSet(byte _DevUID, int value) override { 
        Serial.print(F("Rx Device Set on DevUID=")); Serial.print(_DevUID); 
        Serial.print(F(", to Value = ")); Serial.println(value); 
    }
    virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) override {
      // Record supplied values of a remote node.
    }
    virtual bool TxReady(uint16_t _ToRFAddr) override { return true;} //Simulation; actual radio would need checks.
    virtual void TxData(byte _TxByte) override { Serial1.write(_TxByte); }
    virtual byte pmem_read(int addr) override { return EEPROM.read(addr); }
    virtual void pmem_write(int addr, byte val) override { EEPROM.update(addr, val); }
};
*/
/*
// -------------------------------------------------------------------------------------------------
// RAM-backed instance of DITSEngine HW-Serial 'Call-back' used for simulation and testing (no EEPROM wear-out)
// -------------------------------------------------------------------------------------------------
class RAMmgrDIT : public DITSEngine {
  public:
    //DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes);
    // Total Persistant Memory Allocated = MaxDITRecords * (NameFieldBytes + 4) + 1
    RAMmgrDIT(HardwareSerial& SerialPort) : DITSEngine(0,40,10), SPort(SerialPort) {memset(RAM_EEPROM, 0xFF, sizeof(RAM_EEPROM));}
  protected:
    virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) override {
      // Record supplied values of a remote node.
    }
    virtual int RxReqDeviceValue(byte ditIdx) override { return 42; }
    virtual void RxReqDeviceSet(byte _DevUID, int value) override { 
        Serial.print(F("INO.RxReqDeviceSet devUID("));Serial.print(_DevUID);Serial.print(") -to- "); Serial.print(value);
    }
    virtual bool TxReady(uint16_t _ToRFAddr) override { 
      return SPort.availableForWrite() > 58; // We know Mega UART has 64b buffer so okay to use this.
    } //Simulation; actual radio would need checks.
    virtual void TxData(byte _TxByte) override { SPort.write(_TxByte); }
    virtual byte pmem_read(int addr) override { return RAM_EEPROM[addr]; }
    virtual void pmem_write(int addr, byte val) override { RAM_EEPROM[addr] = val; }
    virtual void RxDITUpdate(byte nodeIdx) override {
      // A call-back to notify implementer Rx has changed the DIT records for 'nodeIdx'.
      // This is useful for keeping a display updated with new changes occuring behind the scenes through Rx communication.
      // Note:  This is only called via Rx.  Implementer DIT changes do not trigger this call-back
    }
  private: 
    byte RAM_EEPROM[0x0300];
    HardwareSerial& SPort;
};
*/
// -------------------------------------------------------------------------------------------------
// RAM-backed instance of DITSEngine SW Loob-Back used for simulation and testing (no EEPROM wear-out)
// -------------------------------------------------------------------------------------------------
class RAMLBmgrDIT : public DITSEngine {
  public:
    //DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes);
    // Total Persistant Memory Allocated = MaxDITRecords * (NameFieldBytes + 4) + 1
    RAMLBmgrDIT() : DITSEngine(0,40,10) {memset(RAM_EEPROM, 0xFF, sizeof(RAM_EEPROM));}
    void ConnectTo(DITSEngine& _pOtherDitsObj) { pOtherDitsObj=&_pOtherDitsObj; }
  protected:
    virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) override {
      // Record supplied values of a remote node.
    }
    virtual int RxReqDeviceValue(byte ditIdx) override { return 42; }
    virtual void RxReqDeviceSet(byte _DevUID, int value) override { 
        Serial.print(F("INO.RxReqDeviceSet devUID("));Serial.print(_DevUID);Serial.print(") -to- "); Serial.print(value);
    }
    virtual bool TxReady(uint16_t _ToRFAddr) override { 
      return true; //SPort.availableForWrite() > 58; // We know Mega UART has 64b buffer so okay to use this.
    } //Simulation; actual radio would need checks.
    virtual void TxData(byte _TxByte) override { if(pOtherDitsObj) pOtherDitsObj->RxData(_TxByte); } // SPort.write(_TxByte); }
    virtual byte pmem_read(int addr) override { return RAM_EEPROM[addr]; }
    virtual void pmem_write(int addr, byte val) override { RAM_EEPROM[addr] = val; }
    virtual void RxDITUpdate(byte nodeIdx) override {
      // A call-back to notify implementer Rx has changed the DIT records for 'nodeIdx'.
      // This is useful for keeping a display updated with new changes occuring behind the scenes through Rx communication.
      // Note:  This is only called via Rx.  Implementer DIT changes do not trigger this call-back
    }
  private: 
    byte RAM_EEPROM[0x0300];
    DITSEngine* pOtherDitsObj = nullptr;
};
//EEmgrDIT  Radio1;
RAMLBmgrDIT Radio1;
RAMLBmgrDIT Radio2;

char* tmpname; // General use array for DIT get Names

//UI
Adafruit_ST7735 oDSP(DISPPINS); //TFT(cs, dc(rs)(cd), rst)
DitsUI dUI;
KeyPad* ButtonCapture = nullptr;
Ebyte oEbyte(RFPINS);
uint32_t msTimer = 0;
int iTestStep = 0;


//-------------------------------------
// Debug Displays to track Test Steps
//-------------------------------------
void DumpRadio1DITS() {
  Serial.println(F("\nnvvvvvvvvvvvvvvvvvvvvvvvvvv RADIO1 DITS vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
  // Step 1: Iterate through Nodes
  DITSEngine::NodeDIT node = Radio1.Node();
  while (node.Next()) {
    node.NGetName(tmpname);
    Serial.print(F("NODE ["));Serial.print(node.NodeIdx());Serial.print(F("] "));
    Serial.print(tmpname);Serial.print(F(" (RF: 0x"));Serial.print(node.NRFAddr(), HEX);
    Serial.print(F(") (NDITVer: "));Serial.print(node.NDITVer());Serial.println(")");
   // Step 2: Iterate through ALL devices to find children of this node
    DITSEngine::DeviceDIT dev = Radio1.Device();
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
void DumpRadio2DITS() {
    Serial.println(F("\nvvvvvvvvvvvvvvvvvvvvvvvvvv RADIO2 DITS vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
    // Step 1: Iterate through Nodes
    DITSEngine::NodeDIT node = Radio2.Node();
    while(node.Next()) {
      node.NGetName(tmpname);
      Serial.print(F("NODE ["));Serial.print(node.NodeIdx());Serial.print(F("] "));
      Serial.print(tmpname);Serial.print(F(" (RF: 0x"));Serial.print(node.NRFAddr(), HEX);
      Serial.print(F(") (NDITVer: "));Serial.print(node.NDITVer());Serial.println(")");
      DITSEngine::DeviceDIT dev = Radio2.Device();
      while(dev.Next(node.NodeIdx())) {     // List Devices matching node.
        dev.DGetName(tmpname);
        Serial.print(F("  |-- DevUID: "));Serial.print(dev.DevUID());
        Serial.print(F(" | Type: "));Serial.print(dev.DevType(), HEX);
        Serial.print(F(" | Attr: 0x"));Serial.print(dev.DevAttr(), HEX);
        Serial.print(F(" | Name: "));Serial.println(tmpname);
      };
    };
    Serial.println(F("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"));
}

void setup() {
  Radio1.ConnectTo(Radio2);
  Radio2.ConnectTo(Radio1);

  tmpname = new char[Radio1.DITNameChar];
  Serial.begin(115200);
  Serial.println(F("DitsUI Test"));

  Radio1.begin();               // Start RF-DITS protocol on sim Radio1
  Radio2.begin();               // Start RF-DITS protocol on sim Radio2

  Serial1.begin(9600);          // Start Radio Communications for Radio1
  Serial2.begin(9600);          // Start Radio Communications for Radio2
  oEbyte.Begin();
  /*
  Variant	Initialization Flag	Notes
  Black Tab	    INITR_BLACKTAB	  Most common 1.8″ variant
  Green Tab	    INITR_GREENTAB	  Different pixel offsets
  Green Tab 2	  INITR_GREENTAB2	  Alternative green tab version
  Red Tab	      INITR_REDTAB	    RGB color order differences
  Mini 160×80	  INITR_MINI160x80	Smaller form factor display
  144 Green Tab	INITR_144GREENTAB	1.44″ square display (128×128)
  */
  oDSP.initR(INITR_BLACKTAB);   //Initialize ST7735 of the GFX
  
  ButtonCapture = new KeyPad(/*Debounce*/300, /*HoldRepeat*/2000, /*BuzzPin*/0xFF);
  ButtonCapture->StartCapture();

  // Standard RF-DITS protocol settings.
  Radio1.RadioPktMaxBytes(58);          // 58 is the default.  This can be deleted.
  Radio1.RxExpireMS(500);               // 30s is the default.  We keep small for wired-comm.
  Radio1.UpdateThisNodeName("Radio1");  // Make sure this node has a readable name.
  Radio1.UpdateThisNodeRFAddr(0x0001);  // Make sure DITS has the correct RF Address on record.
  Radio1.SecNetCode(0);                 // Assign the 'User' network security code.

  Radio2.RadioPktMaxBytes(58);
  Radio2.RxExpireMS(500);
  Radio2.UpdateThisNodeName("Radio2");
  Radio2.UpdateThisNodeRFAddr(0x0002);
  Radio2.SecNetCode(0);

  // Add some devices to see
  Radio1.AddThisNodeDevice(DT_ONOFF, DRW, "devonoff");
  Radio1.AddThisNodeDevice(DT_ANAINPUT, DRO, "devtemp");
  Radio2.AddThisNodeDevice(DT_ONOFF, DRW, "2onoff1");
  Radio2.AddThisNodeDevice(DT_ONOFF, DRW, "2onoff2");

  dUI.Begin(Radio1, oDSP, oEbyte);      //Start DitsUI
}

void loop() {
  // put your main code here, to run repeatedly:
  int iButton = ButtonCapture->Button();
  if(iButton!=NAVKEYNONE) {
    //Serial.println("Button Pressed.");
    dUI.ButtonPress(iButton);
  }
  if (millis()-msTimer >= 2000) { 
    msTimer=millis(); iTestStep++; 
    if(iTestStep==3) {
      Radio1.TxAddRemoteNode(0x0002);
      DumpRadio1DITS(); DumpRadio2DITS();
    }
  }
  Radio1.ProcessLoop();
  Radio2.ProcessLoop();
  while (Serial1.available()>0) {Radio1.RxData(Serial1.read());}
  while (Serial2.available()>0) {Radio2.RxData(Serial2.read());}  
}
