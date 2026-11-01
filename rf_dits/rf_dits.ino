/******************************************************************************************************************//**
 * @file      rf_dits.ino
 * @brief     Clean RF-DITS protocol example using EEPROM and RAM managers
 *********************************************************************************************************************/
#include <EEPROM.h>
#include "rf_dits.h"
#include "DB.h"

// Testing two radios on one arduino mega.
// Wire TX1(pin18) to RX2(pin17)
// Wire TX2(pin16) to RX1(pin19)

///@defgroup DT [DT] Device-Type enumeration.
/// DITS provides 1-byte to designate Device Type definitions.
/// Never (0) as RF uses null-termination '\0' to seperate records.
/// Prevent usage of (0xF?) packet types as they help signal beginning of packets.
///@{
// 0x[7] Device Types
#define DT_ONOFF        0x7E    ///< On/Off Switching Device
#define DT_DIST         0x7A    ///< Distance Sensing Device
#define DT_STSTP3W      0x79    ///< Start Stop 3-Wire
#define DT_ANAINPUT     0x78    ///< Analog Device (Pressure, Temperature)
#define DT_ANAOUTPUT    0x77    ///< Analog Output (PWM control)
#define DT_DLSETPOINT   0x76    ///< Dev Logic Control Boundary
#define DT_DLMATH       0x75    ///< Dev Logic Compare / Math Device

#define DT_MINBOUNDARY  0x75    ///< MINIMUM BOUNDARY CHECK FOR ADD DEVICE KEYS
///@}

///@defgroup DCSP [DT] Device Control Set Point enumeration. NOT part of DITS. 
///@{
#define DCSPO_MIN   0
#define DCSPO_OFF   0    ///< Less Than or Equal To
#define DCSPO_ON    1    ///< Greater Than or Equal To
#define DCSPO_INCR  2    ///< Equal To
#define DCSPO_DECR  3
#define DCSPO_MAX   3
///@}

///@defgroup DCLM [DT] Device Control Logic Mathematics. NOT part of DITS     
///@{
#define DCLM_ADD        0x00    ///< Less Than or Equal To
#define DCLM_SUBTRACT   0x01    ///< Greater Than or Equal To
#define DCLM_MULTIPLY   0x02    ///< Equal To
#define DCLM_DIVIDE     0x03    ///< Not Equal To
#define DCLM_AVERAGE    0x04
#define DCLM_COPY       0x05
#define DCLM_ICOPY      0x06
///@}

uint32_t msTimer = 0;   
int iTestStep = 0;

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

// -------------------------------------------------------------------------------------------------
// RAM-backed instance of DITSEngine 'Call-back' used for simulation and testing (no EEPROM wear-out)
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

//EEmgrDIT  Radio1;
RAMmgrDIT Radio1(Serial1);
RAMmgrDIT Radio2(Serial2);
char* tmpname; // General use array for Names

// -------------------------------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------------------------------
void setup()
{
    tmpname = new char[Radio1.DITNameChar];
    Serial.begin(115200);
    Serial.println(F("RF-DITS Clean Test"));

    Radio1.begin();   // You !MUST! call begin() for DITS to work.
    Radio2.begin();
    Serial1.begin(9600);
    Serial2.begin(9600);
    
    // Standard settings.
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
}
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
//-------------------------------------
// Run Test Steps
//-------------------------------------
void RunTestStep() {
    iTestStep++;

    // Dump DITS on odd numbers.  Check engine changes after commands.
    if (iTestStep & 0x01) { DumpRadio1DITS(); DumpRadio2DITS(); }

    // Add some devices to Radio1
    if (iTestStep==2) {Radio1.AddThisNodeDevice(DT_ONOFF, DRW, "devonoff");}
    if (iTestStep==4) {Radio1.AddThisNodeDevice(DT_ANAINPUT, DRO, "devtemp");}
           
    // Radio2 Requests DITS info from Radio1.
    if (iTestStep==6) {Radio2.TxAddRemoteNode(0x0001);}

    // Radio2 does a does a device setting on Radio1
    if (iTestStep==8) {Radio2.TxSetRemoteDevVal(0x0001, 0, 500);}

    // Radio1 adds another device to it.
    if (iTestStep==10) {Radio1.AddThisNodeDevice(DT_STSTP3W, DRO, "runtdist");}

    // Radio2 tries to do another device setting on Radio1 but because Radio1 has changed
    // its DITS info, by added another device, the next Radio2 attempt to control it 
    // will trigger a mis-match of its 'learned' DITINFO record of Radio1 so Radio1 will send
    // Radio2 a new DITS info packet.
    if (iTestStep==12) {Radio2.TxSetRemoteDevVal(0x0001, 0, 500);}  // trigger a mis-match update (req is tossed).

    // With the new DITS info of Radio1, Radio2 can now successfully make a setting.
    if (iTestStep==14) {Radio2.TxSetRemoteDevVal(0x0001, 0, 500);}  // trigger a mis-match update (req is tossed).

    // Now repeat the mis-match process by deleting a device on Radio1.
    if (iTestStep==16) {Radio1.DelThisNodeDevice(0);}

    // Again when Radio2 tries to control Radio1 a 'leaned' hardward mis-match occurs.
    // So Radio1 sends Radio2 it's 'updated' DITINFO.
    if (iTestStep==18) {Radio2.TxSetRemoteDevVal(0x0001, 0, 500);}
    
    // Radio1 adds a On/Off Read/Write Secure Set Device (to and check Secure Set Feature)
    if (iTestStep==20) {Radio1.AddThisNodeDevice(DT_ONOFF, DRWSS, "onoffSS");}

    // Again when Radio2 trie sto control Radio1 a mis-match (because a device was added) is trigger.
    if (iTestStep==22) {Radio2.TxSetRemoteDevVal(0x0001, 0, 500);}  // trigger a mis-match update (req is tossed).

    // However; Once Radio2 uploads the new Radio1 DITINFO it can now control Radio1.
    // Since the Device being remotely set is an RWSS a 'Secure Set' Challenge is required.
    if (iTestStep==24) {Radio2.TxSetRemoteDevVal(0x0001, 0, 150);}  // now try to set the RWSS

    // Now we can delete the devices on Radio1
    if (iTestStep==26) {
      Radio1.DelThisNodeDevice(0);
      Radio1.DelThisNodeDevice(1);
      Radio1.DelThisNodeDevice(2);
      Radio1.DelThisNodeDevice(3);
    }

    // Repeat Test Steps
    if (iTestStep >= 26) iTestStep=0;
}
// -------------------------------------------------------------------------------------------------
// Loop
// -------------------------------------------------------------------------------------------------
void loop()
{
  if (millis()-msTimer >= 2000) { msTimer=millis(); RunTestStep();}
  Radio1.ProcessLoop();
  Radio2.ProcessLoop();
  while (Serial1.available()>0) {Radio1.RxData(Serial1.read());}
  while (Serial2.available()>0) {Radio2.RxData(Serial2.read());}
}
