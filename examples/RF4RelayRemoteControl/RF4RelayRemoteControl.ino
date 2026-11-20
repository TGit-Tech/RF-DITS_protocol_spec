/******************************************************************************************************************//**
 * @file      RF4RelayRemoteControl.ino
 * @brief     RF-DITS protocol Controlling 4-Relays by Remote Control Example.
 *********************************************************************************************************************/
#include <rf_dits.h>

#define RAMEEPROM           // Use RAM as EEPROM; applies to (1) or (2) DITS Engines.
#define RELAYNODE           // On Compile: Comment out for Handheld.  Leave for Node connected to Relays.
#define RFSERIAL Serial3    // Assign Serial RF-Radio is attached too.

// Notice - Changing these will make previous records void
#define STARTADDR 0
#define MAXDITRECORDS 40
#define NAMEBYTES 10

#if defined(RAMEEPROM)
  #define PMEMREAD(a) RAM_EEPROM[(a)]
  #define PMEMWRITE(a,v) RAM_EEPROM[(a)]=(v)
#else
  #include <EEPROM.h>
  #define PMEMREAD(a) EEPROM.read((a))
  #define PMEMWRITE(a,v) EEPROM.update((a),(v))
#endif
//vvvvvvvvvvvvvvvvvvvv( RFDITS Serial Implementation )vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
class RFDITS : public DITSEngine {
  public:
    //DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes);
    RFDITS(HardwareSerial& SerialPort) : DITSEngine(STARTADDR,MAXDITRECORDS,NAMEBYTES), SPort(SerialPort) {
    #if defined(RAMEEPROM)
      memset(RAM_EEPROM, 0xFF, sizeof(RAM_EEPROM));
    #endif
    }
  protected:
    virtual int RxReqDeviceValue(byte _DevUID) override { return 42; }
    virtual void RxReqDeviceSet(byte _DevUID, int value) override { 
      for(uint8_t i=0;i<4;i++) {
        if(_DevUID==Relay_devUID[i]) { digitalWrite(i+18,value); } // Relay Pins 18,19,20,21
        DBHAAINFO((_DevUID,HEX),(value),("RxReqDeviceSet(DevUID,value)\n"))
      } 
    }
    virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) override {
      // Record supplied values of a remote node.
    }
    virtual bool TxReady(uint16_t _ToRFAddr) override { return (SPort.availableForWrite()>58); }
    virtual void TxData(byte _TxByte) override { SPort.write(_TxByte); }
    virtual byte pmem_read(int addr) override { return PMEMREAD(addr); }
    virtual void pmem_write(int addr, byte val) override { PMEMWRITE(addr, val); }
  private:
    HardwareSerial& SPort;
    #if defined(RAMEEPROM)
      byte RAM_EEPROM[0x0300];
    #endif
};//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

RFDITS DITS(RFSERIAL);
uint32_t msTimer = 0;   
int iTestStep = 0;
char* tmpname;            // General use array for DIT get Names
uint8_t Relay_devUID[4];

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

    Relay_devUID[0] = DITS.AddThisNodeDevice(DT_ONOFF, 0x00, "Relay1");
    Relay_devUID[1] = DITS.AddThisNodeDevice(DT_ONOFF, 0x00, "Relay2");
    Relay_devUID[2] = DITS.AddThisNodeDevice(DT_ONOFF, 0x00, "Relay3");
    Relay_devUID[3] = DITS.AddThisNodeDevice(DT_ONOFF, 0x00, "Relay4");
  #else
    DITS2.RadioPktMaxBytes(58);
    DITS2.RxExpireMS(500);
    DITS2.UpdateThisNodeName("Handheld");
    DITS2.UpdateThisNodeRFAddr(0x0002);
    DITS2.SecNetCode(0);

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
  DITS.ProcessLoop();
}
