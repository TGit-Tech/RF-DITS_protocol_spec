/******************************************************************************************************************//**
 * @file      RF4RelayRemoteControl.ino
 * @brief     RF-DITS protocol Controlling 4-Relays by Remote Control Example.
 *********************************************************************************************************************/
#include <rf_dits.h>
#include <TGTDB.h>

// Tested using Serial1 on two Mega 2560 boards with a crossed UART jumper.
// Connect:
//   RELAYNODE pin 18 (TX1) -> Handheld pin 19 (RX1)
//   Handheld  pin 18 (TX1) -> RELAYNODE pin 19 (RX1)
//
// Each Mega is powered independently by USB.
// No GND connection between boards is required if using USB.
//
// Compile the same sketch for both Mega's:
//   RELAYNODE defined     = Relay Node
//   RELAYNODE commented   = Handheld
#define RAMEEPROM           // Use RAM as EEPROM
//#define RELAYNODE           // On Compile: Comment out for Handheld.  Leave for Node connected to Relays.
#define RFSERIAL Serial1    // Assign Serial RF-Radio is attached too.

// Notice - Changing these will make previous records void
#define STARTADDR 0
#define MAXDITRECORDS 40
#define NAMEBYTES 10
#define DT_ONOFF        0x7E    ///< On/Off Switching Device

#if defined(RAMEEPROM)
  #define PMEMREAD(a) RAM_EEPROM[(a)]
  #define PMEMWRITE(a,v) RAM_EEPROM[(a)]=(v)
#else
  #include <EEPROM.h>
  #define PMEMREAD(a) EEPROM.read((a))
  #define PMEMWRITE(a,v) EEPROM.update((a),(v))
#endif
#define MAXDEVICES 30
#define MAXREMVALS 50
#define RDEVLOADED 0x01

// Define a Device Interface for all Devices.
class LocalDeviceI {
public:
  LocalDeviceI(uint8_t _devUID): devUID(_devUID) {}
  virtual uint16_t Value() = 0;
  virtual bool Value(uint16_t _Value) = 0;
  virtual void Loop() {}
  virtual uint8_t DevUID() { return devUID; }
protected:
  uint8_t devUID = BNONE;  
};

// Define OnOff
class OnOff : public LocalDeviceI {
public:
  OnOff(uint8_t _devUID, uint8_t _Pin, uint8_t _pnMode, bool _ActiveLow)
  : LocalDeviceI(_devUID), Pin(_Pin), pnMode(_pnMode), ActiveLow(_ActiveLow) {}
  uint8_t Pin = 0;
  uint8_t pnMode = 0;
  bool ActiveLow = false;
  uint16_t Value() override {
    pinMode(Pin, pnMode);
    bool val = digitalRead(Pin);
    return (ActiveLow) ? !val : val;
  }
  bool Value(uint16_t _Value) override {
    pinMode(Pin, pnMode);
    bool val = (_Value != 0);
    val = (ActiveLow) ? !val : val;
    digitalWrite(Pin, val);
    DBHAAINFO((Pin),(val),("OnOff::Value[SET] digitalWrite(Pin,val)\n"))
    return true;
  }
};

//-------------------------------------------------------------------------------------------------
// DITDevices - 
// - RFDITS Serial Implementation
// - Local Device Container.  So far best approach due to call-for Device Values.
// - Remote Values Received Buffer.
//-------------------------------------------------------------------------------------------------
class DITDevices : public DITSEngine {
  public:
    //DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes);
    DITDevices(HardwareSerial& SerialPort) : DITSEngine(STARTADDR,MAXDITRECORDS,NAMEBYTES), SPort(SerialPort) {
    #if defined(RAMEEPROM)
      memset(RAM_EEPROM, 0xFF, sizeof(RAM_EEPROM));
    #endif
    }
    uint16_t Value(uint8_t _devUID) {
      for(uint8_t i=0;i<LDidx;i++) {
        if(LDevice[i]->DevUID()==_devUID) { 
          uint16_t RtnValue = LDevice[i]->Value();
          DBHAAINFO((_devUID,HEX),(RtnValue),("DITDevices::Value[GET](DevUID)(RtnValue)\n"))
          return RtnValue;
        }
      }
    }
    bool Value(uint8_t _devUID, uint16_t _Value) {
      for(uint8_t i=0;i<LDidx;i++) {
        if(LDevice[i]->DevUID()==_devUID) { 
          bool RtnValue = LDevice[i]->Value(_Value);
          DBHAAAINFO((_devUID,HEX),(_Value),(RtnValue),("DITDevices::Value(DevUID,[SET]Value)(RtnValue)\n"))
          return RtnValue;
        }
      }
    }
    uint8_t NewOnOffDevice(const char* _Name, uint8_t _Pin, uint8_t _pnMode, bool _ActiveLow = false) {
      if(LDidx>=MAXDEVICES) return BNONE;
      uint8_t DITDevUID = this->AddThisNodeDevice(DT_ONOFF, 0x00, _Name); // Add a DIT Record.
      LDevice[LDidx] = new OnOff(DITDevUID,_Pin,_pnMode,_ActiveLow);      // Add device obj to Local Device collection.
      LDidx++; return DITDevUID;
    }
    // Remote Device Values Buffer
    uint8_t RNodeIdx = 0;
    void RdevValReset() { RNodeIdx=0; for(uint8_t i=0; i<MAXREMVALS; i++){RdevUID[i]=0;RdevVal[i]=0;} }
    // ProcessLoop()
    uint8_t ProcLoop() {
      while (RFSERIAL.available()>0) {this->RxData(RFSERIAL.read());}
      this->ProcessLoop();
      return (Rdevidx!=0) ? RDEVLOADED : BNONE;
    }
  protected:
    virtual int RxReqDeviceValue(byte _DevUID) override { 
      for(uint8_t i=0;i<LDidx;i++) {
        if(LDevice[i]->DevUID()==_DevUID) { 
          uint16_t RtnValue = LDevice[i]->Value();
          DBHAAINFO((_DevUID,HEX),(RtnValue),("DITDevices::RxReqDeviceValue(DevUID)(RtnValue)\n"))
          return RtnValue;
        }
      }
    }
    virtual void RxReqDeviceSet(byte _DevUID, int value) override { 
      for(uint8_t i=0;i<LDidx;i++) {
        if(LDevice[i]->DevUID()==_DevUID) { 
          LDevice[i]->Value(value);
          DBHAAINFO((_DevUID,HEX),(value),("DITDevices::RxReqDeviceSet(DevUID,value)\n"))
        }
      } 
    }
    virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) override {
      // Buffer supplied values of a remote node.
      // Acceptance is flagged by setting RNodeIdx=0
      if(Rdevidx>=MAXREMVALS) return;
      if(RNodeIdx!=_NodeIdx && RNodeIdx!=0) return;
      RNodeIdx = _NodeIdx;
      RdevUID[Rdevidx] = _DevUID;
      RdevVal[Rdevidx] = value;
      Rdevidx++;
    }
    virtual bool TxReady(uint16_t _ToRFAddr) override { return true; } //return (SPort.availableForWrite()>58); }
    virtual void TxData(byte _TxByte) override { DBHAINFO((_TxByte,HEX),("TxData")) SPort.write(_TxByte); }
    virtual byte pmem_read(int addr) override { return PMEMREAD(addr); }
    virtual void pmem_write(int addr, byte val) override { PMEMWRITE(addr, val); }
  private:
    HardwareSerial& SPort;
    #if defined(RAMEEPROM)
      byte RAM_EEPROM[0x0300];
    #endif
    LocalDeviceI* LDevice[MAXDEVICES] = {nullptr};
    uint8_t LDidx = 0;
    uint8_t RdevUID[MAXREMVALS] = {0};
    uint16_t RdevVal[MAXREMVALS] = {0};
    uint8_t Rdevidx = 0;
};//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

DITDevices DITS(RFSERIAL);
uint32_t msTimer = 0;   
int iTestStep = 0;
char* tmpname;            // General use array for DIT get Names
uint8_t Relay_devUID[4];
uint8_t Relay1UID=BNONE, Relay2UID=BNONE, Relay3UID=BNONE, Relay4UID=BNONE;

// 0x[7] Device Types
#define DT_ONOFF        0x7E    ///< On/Off Switching Device
#define DT_DIST         0x7A    ///< Distance Sensing Device
#define DT_STSTP3W      0x79    ///< Start Stop 3-Wire
#define DT_ANAINPUT     0x78    ///< Analog Device (Pressure, Temperature)


// -------------------------------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------------------------------
void setup()
{
  tmpname = new char[DITS.DITNameChar];
  Serial.begin(115200);
  Serial.println(F("RF-DITS 4-Relay Remote Controlled Test"));

  RFSERIAL.begin(9600);
  DITS.begin();                      // You !MUST! call begin() for DITS to work.
  
  #if defined(RELAYNODE)
    DITS.RadioPktMaxBytes(58);          // 58 is the default.  This can be deleted.
    DITS.RxExpireMS(500);               // 30s is the default.  We keep small for wired-comm.
    DITS.UpdateThisNodeName("Relays");  // Make sure this node has a readable name.
    DITS.UpdateThisNodeRFAddr(0x0001);  // Make sure DITS has the correct RF Address on record.
    DITS.SecNetCode(0);                 // Assign the 'User' network security code.

    //Add the Relay devices on the RELAYNODE.
    //NewOnOffDevice(const char* _Name, uint8_t _Pin, uint8_t _pnMode, bool _ActiveLow = false)
    Relay1UID = DITS.NewOnOffDevice("Relay1",20,OUTPUT,true);
    Relay2UID = DITS.NewOnOffDevice("Relay2",21,OUTPUT,true);
    Relay3UID = DITS.NewOnOffDevice("Relay3",22,OUTPUT,true);
    Relay4UID = DITS.NewOnOffDevice("Relay4",23,OUTPUT,true);

    //Do a Local Relay Switch Test
    //Value(uint8_t _devUID, uint16_t _Value)
    DITS.Value(Relay1UID,1);
    delay(1000);
    DITS.Value(Relay1UID,0);
  #else
    DITS.RadioPktMaxBytes(58);
    DITS.RxExpireMS(500);
    DITS.UpdateThisNodeName("Handheld");
    DITS.UpdateThisNodeRFAddr(0x0002);
    DITS.SecNetCode(0);
  #endif

}
//-------------------------------------
// Debug Displays to track Test Steps
//-------------------------------------
void DumpDITS() {
  Serial.println(F("\nnvvvvvvvvvvvvvvvvvvvvvvvvvv RADIO1 DITS vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
  // Step 1: Iterate through Nodes
  DITSEngine::NodeDIT node = DITS.Node();
  while (node.Next()) {
    node.NGetName(tmpname);
    Serial.print(F("NODE ["));Serial.print(node.NodeIdx());Serial.print(F("] "));
    Serial.print(tmpname);Serial.print(F(" (RF: 0x"));Serial.print(node.NRFAddr(), HEX);
    Serial.print(F(") (NDITVer: "));Serial.print(node.NDITVer());Serial.println(")");
   // Step 2: Iterate through ALL devices to find children of this node
    DITSEngine::DeviceDIT dev = DITS.Device();
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

//-------------------------------------
// Run Test Steps
//-------------------------------------
void RunTestStep() {
    iTestStep++;

    if (iTestStep==1) { DumpDITS(); } // Before anything.

  #if !defined(RELAYNODE) // For handheld...        
    
    // Requests DITS info from RFAddr0x0001 (RELAYNODE).
    if (iTestStep==2) {DITS.TxAddRemoteNode(0x0001);}
    if (iTestStep==3) { DumpDITS(); }
    
    // After getting RELAYNODE DIT Records; store its devUID(s) for access
    if (iTestStep==4) {
      DITSEngine::NodeDIT node = DITS.Node();
      while(node.Next()) {
        if(node.NRFAddr()==0x0001) {
          
          uint8_t devCnt = 0;
          DITSEngine::DeviceDIT dev = DITS.Device();
          while (dev.Next(node.NodeIdx())) {
            Relay_devUID[devCnt] = dev.DevUID(); devCnt++;
            if(devCnt>3) break;
          }
        
        }
      }
    }

    // Relay1 On(1) on RFAddr0x0001 (RELAYNODE), given devUID[0].
    if (iTestStep==5) {DITS.TxSetRemoteDevVal(0x0001,  Relay_devUID[0], 1);}

    // Relay1 Off on RELAYNODE
    if (iTestStep==6) {DITS.TxSetRemoteDevVal(0x0001, Relay_devUID[0], 0);}
    
    // Relay2 On on RELAYNODE
    if (iTestStep==7) {DITS.TxSetRemoteDevVal(0x0001, Relay_devUID[1], 1);}
    
    // Relay2 Off on RELAYNODE
    if (iTestStep==8) {DITS.TxSetRemoteDevVal(0x0001, Relay_devUID[1], 0);}
    
    // Relay3 On on RELAYNODE
    if (iTestStep==9) {DITS.TxSetRemoteDevVal(0x0001, Relay_devUID[2], 1);}
    
    // Relay3 Off on RELAYNODE
    if (iTestStep==10) {DITS.TxSetRemoteDevVal(0x0001, Relay_devUID[2], 0);}
    
    // Relay4 On on RELAYNODE
    if (iTestStep==11) {DITS.TxSetRemoteDevVal(0x0001, Relay_devUID[3], 1);}
    
    // Relay4 Off on RELAYNODE
    if (iTestStep==12) {DITS.TxSetRemoteDevVal(0x0001, Relay_devUID[3], 0);}
    
  #endif

    // Repeat Test Steps - Adding Same Remote DIT records should update not add a second set.
    if (iTestStep >= 30) iTestStep=0;
}
// -------------------------------------------------------------------------------------------------
// Loop
// -------------------------------------------------------------------------------------------------
void loop()
{
  if (millis()-msTimer >= 2000) { msTimer=millis(); RunTestStep();}
  if(DITS.ProcLoop()==RDEVLOADED) {
    // Remote Device Value Buffer has Values
  }
}
