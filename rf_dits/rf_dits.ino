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
    RAMmgrDIT() : DITSEngine(0,140,10) {}
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

EEmgrDIT  Radio1;
RAMmgrDIT Radio2;
//void UpdateThisNode(const char* name, uint16_t RFAddr);
//bool AddThisNodeDevice(const char* name, byte DevType);
//bool DelThisNodeDevice(byte devIdx);
//void ProcessLoop();
//byte SecNetCode();
//bool SecNetCode(byte _SecNetCode);
//void RxRadioData(byte _byte);

// -------------------------------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    Serial.println(F("RF-DITS Clean Test"));

    Serial1.begin(9600);
    Serial2.begin(9600);

    //UpdateThisNode(const char* name, uint16_t RFAddr)
    Radio1.UpdateThisNode("Radio1", 0x0001);    // Must be unique.
    Radio2.UpdateThisNode("Radio2", 0x0002);    // Must be unique.
    Radio1.SecNetCode(0x4F); // Must match.
    Radio2.SecNetCode(0x4F);

    randomSeed(analogRead(A0));
}

// -------------------------------------------------------------------------------------------------
// Loop
// -------------------------------------------------------------------------------------------------
void loop()
{
    if (Serial1.available()) { Radio1.RxData(Serial1.read()); }
    if (Serial2.available()) { Radio2.RxData(Serial2.read()); }
}
