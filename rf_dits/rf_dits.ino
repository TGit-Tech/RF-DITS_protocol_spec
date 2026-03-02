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
/******************************************************************************//**
 * @defgroup DT [DT] Device-Type enumeration. Part of DITS protocol.
 *  - You get 1-byte (not 0) to designate Device Type definitions.
 *  - NO NOT USE '0'.  RF uses null-termination '\0' to seperate records.  @{
 *********************************************************************************/
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
/******************************************************************************//**
 * @defgroup DA [DA] Device-Attributes.
 *  - You get 1-byte to add attributes to your Device Types.  You can use 0.
 *********************************************************************************/
// 0x[7] Device Types                  WS
#define DA_RO           0x00    ///< 0b00xx xxxx Read-Only Device
#define DA_RW           0x40    ///< 0b01xx xxxx R/Write Device no SS
#define DA_RWSS         0x80    ///< 0b10xx xxxx R/Write with (SS)Secured Set (DITS specific).
#define DA_UNDEF        0xC0    ///< 0b11xx xxxx Undefined
#define ACCESS_MASK     0xC0    ///< 0bxx11 xxxx Mask bits 7,6
#define DA_SCALE1       0x00    ///< 0bxx00 xxxx No Value Scale
#define DA_SCALE10      0x10    ///< 0bxx01 xxxx Scale Value x10.
#define DA_SCALE100     0x20    ///< 0bxx10 xxxx Scale Value x100
#define DA_SCALE1000    0x30    ///< 0bxx11 xxxx Scale Value x1000
#define SCALE_MASK      0x30    ///< 0bxx11 xxxx Mask bits 5,4

#define DT_MINBOUNDARY  0x75    ///< MINIMUM BOUNDARY CHECK FOR ADD DEVICE KEYS
///@}
/******************************************************************************//**
 * @defgroup DCSP [DT] Device Control Set Point enumeration. NOT part of DITS. @{
 *********************************************************************************/
#define DCSPO_MIN   0
#define DCSPO_OFF   0    ///< Less Than or Equal To
#define DCSPO_ON    1    ///< Greater Than or Equal To
#define DCSPO_INCR  2    ///< Equal To
#define DCSPO_DECR  3
#define DCSPO_MAX   3
///@}
/******************************************************************************//**
 * @defgroup DCLM [DT] Device Control Logic Mathematics. NOT part of DITS     @{
 *********************************************************************************/
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
    
    Radio1.AddThisNodeDevice(DT_ONOFF, DA_RW, "devonoff");   // Add devices to Radio1.
    Radio1.AddThisNodeDevice(DT_ANAINPUT, DA_RO, "devtemp");
    
    

    // Check DIT Tables exist and have data.
    Serial.print("SecNet1: ");Serial.println(Radio1.SecNetCode());
    Serial.print("SecNet2: ");Serial.println(Radio2.SecNetCode());
    DITSEngine::NodeDIT node1 = Radio1.Node(0);
    DITSEngine::NodeDIT node2 = Radio2.Node(0);
    node1.NGetName(tmpname);Serial.print("node1 name: ");Serial.println(tmpname);
    node2.NGetName(tmpname);Serial.print("node2 name: ");Serial.println(tmpname);
    char hexStr1[8];sprintf(hexStr1, "0x%04X", node1.NRFAddr());
    char hexStr2[8];sprintf(hexStr2, "0x%04X", node2.NRFAddr());
    Serial.print("node1 RF-Addr: ");Serial.println(hexStr1);
    Serial.print("node2 RF-Addr: ");Serial.println(hexStr2);

    // Test Rx.
    // Send 'testaddnode1dev1' dummy packet to test Rx DITINFO adding.
    //for(int i=0; i<sizeof(testaddnode1dev1); i++) {
      //Radio1.RxData(testaddnode1dev1[i]);
    //}

    // Test Tx
    //Radio1.TxSetRemoteDevVal(uint16_t nodeAddr, byte ditIdx, int value)
    //Radio1.TxSetRemoteDevVal(0x0002, 0, 500);
    //TxSetRemoteDevVal

    // from Radio2 request DITINFO on Radio1
    Radio2.TxAddRemoteNode(0x0001);

    //randomSeed(analogRead(A0));
}
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

void RunTestStep() {
    iTestStep++;

    // Dump DITS on odd numbers.  Check engine changes after commands.
    if (iTestStep & 0x01) { DumpRadio1DITS(); DumpRadio2DITS(); }

    // Do a runtime add device and delete to make sure NDITVer mis-match causes an 'update' to trigger.
    if (iTestStep==2) {Radio2.TxSetRemoteDevVal(0x0001, 0, 500);}
    if (iTestStep==4) {Radio1.AddThisNodeDevice(DT_STSTP3W, DA_RW, "runtdist");}

    if (iTestStep==6) {Radio2.TxSetRemoteDevVal(0x0001, 0, 500);}  // trigger a mis-match update (req is tossed).
    if (iTestStep==8) {Radio1.DelThisNodeDevice(0);}
    if (iTestStep==10) {Radio2.TxSetRemoteDevVal(0x0001, 0, 500);}  // trigger a mis-match update (req is tossed).
    
    // Then add a RWSS and check Secure Set
    if (iTestStep==12) {Radio1.AddThisNodeDevice(DT_ONOFF, DA_RWSS, "onoffSS");}
    if (iTestStep==14) {Radio2.TxSetRemoteDevVal(0x0001, 0, 500);}  // trigger a mis-match update (req is tossed).
    if (iTestStep==18) {Radio2.TxSetRemoteDevVal(0x0001, 0, 150);}  // now try to set the RWSS

    // Repeat Test Steps
    if (iTestStep >= 20) iTestStep=0;
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
