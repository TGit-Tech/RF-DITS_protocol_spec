#pragma once

#include <EEPROM.h>
#include <TGTDB.h>

// Notice - Changing these will make previous records void
#define STARTADDR 0
#define MAXDITRECORDS 40
#define NAMEBYTES 10

// Testing and Simulation
//#define RAMEEPROM             // Use RAM as EEPROM
//#define SWSERIALLOOP          // Tie RxData to TxData via 'ConnectTo'
// For HWSerialLoop comment-out SWSERIALLOOP
// and Wire Tx->Rx and Rx->Tx across 2-Different Serial ports (Mega only)

#if defined(RAMEEPROM)
  #define PMEMREAD(a) RAM_EEPROM[(a)]
  #define PMEMWRITE(a,v) RAM_EEPROM[(a)]=(v)
#else
  #define PMEMREAD(a) EEPROM.read((a))
  #define PMEMWRITE(a,v) EEPROM.update((a),(v))
#endif

#if defined(SWSERIALLOOP)
  #define TXREADYLINE (pOtherDitsObj != nullptr)
  #define TXDATALINE if(pOtherDitsObj) pOtherDitsObj->RxData(_TxByte)
#else
  #define TXREADYLINE (SPort.availableForWrite()>58) // Mega UART has 64-byte buffer so okay to use this.
  #define TXDATALINE SPort.write(_TxByte)
#endif

// -------------------------------------------------------------------------------------------------
// RFDITS - A Generic Multi-purpose DITSEngine Call-back Implementation
// -------------------------------------------------------------------------------------------------
class RFDITS : public DITSEngine {
  public:
    //DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes);
    #if defined(SWSERIALLOOP)
      RFDITS() : DITSEngine(STARTADDR,MAXDITRECORDS,NAMEBYTES) {
    #else
      RFDITS(HardwareSerial& SerialPort) : DITSEngine(STARTADDR,MAXDITRECORDS,NAMEBYTES), SPort(SerialPort) {
    #endif
    #if defined(RAMEEPROM)
      memset(RAM_EEPROM, 0xFF, sizeof(RAM_EEPROM));
    #endif
    } // Close Constructor
    #if defined(SWSERIALLOOP)
      void ConnectTo(DITSEngine& _pOtherDitsObj) { pOtherDitsObj=&_pOtherDitsObj; }
    #endif
  protected:
    virtual int RxReqDeviceValue(byte _DevUID) override { return 42; }
    virtual void RxReqDeviceSet(byte _DevUID, int value) override { 
        Serial.print(F("Rx Device Set on DevUID=")); Serial.print(_DevUID); 
        Serial.print(F(", to Value = ")); Serial.println(value); 
    }
    virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) override {
      // Record supplied values of a remote node.
    }
    virtual bool TxReady(uint16_t _ToRFAddr) override { return TXREADYLINE;}
    virtual void TxData(byte _TxByte) override { TXDATALINE; }
    virtual byte pmem_read(int addr) override { return PMEMREAD(addr); }
    virtual void pmem_write(int addr, byte val) override { PMEMWRITE(addr, val); }
  private:
    #if !defined(SWSERIALLOOP) 
      HardwareSerial& SPort;
    #endif
    #if defined(SWSERIALLOOP)
      DITSEngine* pOtherDitsObj = nullptr;
    #endif
    #if defined(RAMEEPROM)
      byte RAM_EEPROM[0x0300];
    #endif
};