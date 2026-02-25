/******************************************************************************************************************//**
 * @file      rf_dits.ino
 * @brief     Clean RF-DITS protocol example using EEPROM and RAM managers
 *********************************************************************************************************************/
#include <EEPROM.h>
#include "rf_dits.h"
#include "DB.h"

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
 * @defgroup DA [DA] Device-Attributes.  Part of DITS protocol.
 *  - You get 1-byte to add attributes to your Device Types.  You can use 0.
 *********************************************************************************/
// 0x[7] Device Types                  WS
#define DA_RO           0x40    ///< 0b00xx xxxx Read-Only Device
#define DA_RW           0x80    ///< 0b10xx xxxx R/Write Device no SS
#define DA_RWSS         0xC0    ///< 0b11xx xxxx R/Write with (SS)Set Secured.
#define DA_SCALE10      0x79    ///< Scale the Value x10 for floating-point
#define DA_SCALE100     0x78    ///< Scale the Value x100 for floating-point

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
// Only use one of the below.  Both are there to simulate (2)Radio's on one Mega Board.
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

// -------------------------------------------------------------------------------------------------
// RAM-backed instance of DITSEngine 'Call-back' Implementation via 'overloading'
// -------------------------------------------------------------------------------------------------
class RAMmgrDIT : public DITSEngine {
  public:
    //DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes);
    RAMmgrDIT() : DITSEngine(0,140,10) {memset(RAM_EEPROM, 0xFF, sizeof(RAM_EEPROM));}
  protected:
    virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) override {
      // Record supplied values of a remote node.
    }
    virtual int RxReqDeviceValue(byte ditIdx) override { return 42; }
    virtual void RxReqDeviceSet(byte ditIdx, int value) override { 
        Serial.print(F("RAM Set DITidx=")); Serial.print(ditIdx); 
        Serial.print(F(" = ")); Serial.println(value); 
    }
    virtual bool TxReady(uint16_t _ToRFAddr) override { return true;} //Simulation; actual radio would need checks.
    virtual void TxData(byte _TxByte) override { Serial2.write(_TxByte); }
    virtual byte pmem_read(int addr) override { return RAM_EEPROM[addr]; }
    virtual void pmem_write(int addr, byte val) override { RAM_EEPROM[addr] = val; }
  private: 
    byte RAM_EEPROM[0x0800];
};
char tmpname[11]; // Must match _NameFieldBytes +1; just a helper array for char[] names.

//EEmgrDIT  Radio1;
RAMmgrDIT Radio2;

//                           Type, SecH, SecL, RFH,  RFL,  VER , 'N',  'o',  'd',  'e',  '1',  '\0'
byte testaddnode1dev1[20] = {0x4F, 0x02, 0x20, 0x34, 0x4E, 0x01, 0x4E, 0x6F, 0x64, 0x65, 0x31, 0x0,
                  DT_ONOFF, DA_RO, 0x00, 0x64, 0x65, 0x76, 0x31, 0x0};
//                                 dUID,  'd',  'e',  'v',  '1', '\0'
// -------------------------------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    Serial.println(F("RF-DITS Clean Test"));

    Serial1.begin(9600);
    Serial2.begin(9600);

    //DBDITAAAENTER((_pmemBeginAddr),(_MaxDITRecords),(_NameFieldBytes),("DITSEngine::DITSEngine(<BeginAddr>,<MaxDITS>,<NameBytes>)\n"))
    Serial.print(Radio2.pmSecNetAddr);Serial.println(" :pmSecNetAddr");
    Serial.print(Radio2.pmMaxDITRecords);Serial.println(" :pmMaxDITRecords");
    Serial.print(Radio2.pmDITbase);Serial.println(" :pmDITbase");
    Serial.print(Radio2.pmDITend);Serial.println(" :pmDITend");
    Serial.print(Radio2.pmDITEndAddr);Serial.println(" :pmDITEndAddr");
    Serial.print(Radio2.pmDITEndAddr-Radio2.pmSecNetAddr);Serial.println("<< Total Bytes Allocated for DIT Records.");

    //UpdateThisNode(const char* name, uint16_t RFAddr)
    //Radio1.UpdateThisNode("Radio1", 0x0001);    // Must be unique.
    Radio2.UpdateThisNodeName("Radio2");    // Must be unique.
    Radio2.UpdateThisNodeRFAddr(0x0202);
    Radio2.SecNetCode(0);
    //Radio1.SecNetCode(0x4F); // Must match.
    //Radio2.SecNetCode(0x4F);

    // Load some 'nodes' into the DIT Engine.
    // The only way to do this from implemeter-side is send packets.
    /*
    for(DITSEngine::NodeDIT node(0); node.IsValid(); node.NextAll()) {

    }
    */
    Serial.print("SecNet: ");Serial.println(Radio2.SecNetCode());
    DITSEngine::NodeDIT tnode = Radio2.Node(0);
    tnode.NGetName(tmpname);
    Serial.print("the name: ");
    Serial.println(tmpname);
    char hexStr[5];sprintf(hexStr, "0x%04X", tnode.NRFAddr());
    Serial.print("the address: ");Serial.print(hexStr);Serial.println("");
    //tnode = Radio2->Node(0);
    //Serial.println(Radio2->Node(0)->DITidx());

    //Send 'testaddnode1dev1' dummy packet to test Rx DEVITBL adding.
    for(int i=0; i<sizeof(testaddnode1dev1); i++) {Radio2.RxData(testaddnode1dev1[i]);}

    randomSeed(analogRead(A0));
}
void Check() {
    // Check engine added it.
    for(DITSEngine::NodeDIT node = Radio2.Node(0); node.IsValid(); node.Next()) {
      node.NGetName(tmpname);
      Serial.println(tmpname);
    }
    for(DITSEngine::DeviceDIT dev = Radio2.Device(0); dev.IsValid(); dev.Next()) {
      dev.DGetName(tmpname);
      Serial.println(tmpname);
    }
}

uint32_t msTimer = 0;
// -------------------------------------------------------------------------------------------------
// Loop
// -------------------------------------------------------------------------------------------------
void loop()
{
  if (millis()-msTimer >= 1000) {msTimer=millis();Check();}
  Radio2.ProcessLoop();
    //if (Serial1.available()) { Radio1.RxData(Serial1.read()); }
    //if (Serial2.available()) { Radio2.RxData(Serial2.read()); }
}
