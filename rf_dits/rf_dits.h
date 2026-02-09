/******************************************************************************************************************//**
 * @file    rf_dits.h
 * @brief   RF Device-Index-Table with Security protocol
 *********************************************************************************************************************/
#ifndef _RF_DITS_H
#define _RF_DITS_H

#include "DB.h"
#include "EEMap.h"
#include <EEPROM.h>
#include <ArduinoQueue.h>

#define BNONE         0xFF      ///< Byte NONE/UNSET
#define INONE         0xFFFF    ///< Integer NONE/UNSET
#define VALNOTSET     0x8000

/*********************************************//**
 * @defgroup PKT Radio Packet-Types
 * @{
 ************************************************/
#define PKT_NOTSET        0xFF

// THIS-RADIO Requests Packet-Types
#define PKT_GETCONFIG     0x7F    // 0b0111 1111 (bit-5 set)
#define PKT_GETVALS       0x7E    // 0b0111 1110    
#define PKT_GETVAL        0x7D
#define PKT_SETVAL        0x7C

// Replies Packet-Types
#define PKT_GOTCONFIG     0x5F    // 0b0101 1111 (bit-5 unset)
#define PKT_GOTVALS       0x5E
#define PKT_GOTVAL        0x5D    
///@}
/*********************************************//**
 * @defgroup PKB Radio Common PacKet-Bytes by idx
 * @{
 ************************************************/
#define PKB_TO_RFH      0
#define PKB_TO_RFL      1
#define PKB_CHANNEL     2
#define PKB_SECH        3
#define PKB_SECL        4
#define PKB_TYPE        5     // This point forward may differ
#define PKB_FROM_RFH    6
#define PKB_FROM_RFL    7
#define PKB_TODEVER     8
#define PKB_TODECONFIG  9     // Start Tode Config Data
#define PKB_DITidx      9     // First DITidx on GETVALS
#define PKB_VALUEH      10
#define PKB_VALUEL      11
///@}
/*********************************************//**
 * @defgroup PKV Radio Packet Value Constants
 * @{
 ************************************************/
#define PKV_EXPIRE_TIME  30000   ///< How long a GOT value till marked expired 
///@}

//#####################################################################################################
/******************************************************************************************************************//**
 * @class   RxPacket
 * @brief   Receiving Byte Protocol for RF-Radio
 *********************************************************************************************************************/
class RxPacket {
  public:
    RxPacket();

    bool          RxByte(byte _Byte);                                     ///< Returns 'true' at end of Packet
    byte          Size = 0;

    bool          IsValid();
    byte          Type();
    int           FromRF();
    byte          Version();
    byte          DITidx();

    int                   SetValue();                                       ///< PKT_SETVAL Value
    int                   Value(byte _DITidx);
    unsigned long         PacketStartTimeMS = 0;
    void                  SaveTodeConfig(int _EEAddress);
    
    ArduinoQueue<byte>*   ExtraBytes = 0;
    
    byte    Bytes[58] = {255,255,255,255,255,255,255,255,255,255,
                         255,255,255,255,255,255,255,255,255,255,
                         255,255,255,255,255,255,255,255,255,255,
                         255,255,255,255,255,255,255,255,255,255,
                         255,255,255,255,255,255,255,255,255,255,
                         255,255,255,255,255,255,255,255};

  private:
    bool    IsSecure();
    int     NextIdx = 3;         // Start at 3 No-To(0,1)/Chn(2) (0 - 57)...[Idx.0 - 57]
};
/******************************************************************************************************************//**
 * @class   TxPacket
 * @brief   Transmission Byte Protocol for RF-Radio
 * @todo    Implement nonce-milli() with SecNet for replay attacks on SETVAL only
 *********************************************************************************************************************/
class TxPacket {
  public:
    TxPacket(byte _SecNet, byte _Type, int _ToRF, byte _Ver=BNONE, byte _DevDITidx=BNONE, int _Value = INONE);

    void          TxByte(byte _Byte);
    int           Secure();
    int           Size = 0;
    void          AddTodeConfig(int _EEAddress);
    void          AddValue(byte _DITidx, int _Value);
    
    ArduinoQueue<byte>*   ExtraBytes = 0;
        
    byte    Bytes[58] = {255,255,255,255,255,255,255,255,255,255,
                         255,255,255,255,255,255,255,255,255,255,
                         255,255,255,255,255,255,255,255,255,255,
                         255,255,255,255,255,255,255,255,255,255,
                         255,255,255,255,255,255,255,255,255,255,
                         255,255,255,255,255,255,255,255};
                         
  private:
    
    int     ByteIdx = 0;  
};
/******************************************************************************************************************//**
 * @class   RadioI
 * @brief   Common Interface Base-Class for various RF-Radios
 *********************************************************************************************************************/
class RadioI {
  public:
    virtual void          Address(unsigned int _RFAddress) {};  ///< SET Radio RF-Address
    virtual unsigned int  Address() {};                         ///< GET Radio RF-Address
    virtual void          AirSpeed(byte _Speed);                ///< SET Radio AirSpeed[3]
    virtual byte          AirSpeed();                           ///< GET Radio AirSpeed[3]
    virtual void          Frequency(int _RFFrequency) {};       ///< SET Radio Frequency Setting
    virtual int           Frequency() {};                       ///< GET Radio Frequency Setting
    virtual void          TxPower(byte _TxPower){};             ///< SET Radio Tx Power Setting
    virtual byte          TxPower(){};                          ///< GET Radio Tx Power Setting

    virtual bool          PacketAvailable(){};
    virtual void          Send(TxPacket* Tx){};
};
//_____________________________________________________________________________________________________________________

#endif
