///@file    SETGS.h
///@brief   Device Local Settings in Persistant Memory class.
#pragma once
#include <Arduino.h>
#if __has_include("Config.h")
    #include "Config.h"
#endif
#include <TGTDB.h>
#include <BitFields.h>

#define BNONE     0xFF    ///< Byte NONE/UNSET
#define INONE     0xFFFF  ///< Integer NONE/UNSET
#define SINONE    0x8000  ///< Signed Integer NONE/UNSET

//Setpoint 'Enum Timeframe' Multiply all by 2,000 for ms.
void InitRAMMem();

/// @class SETGSDigital
/// @brief Digital Device Storage / Settings (SETG) in PMEM
class SETGSDigital {
  private:
    int args[5];
    BitField bf;
    int devAddr = INONE;
    int empty = INONE;
  public:
    /// @brief Scans the PMEM for existing devUID or finds a spot.
    /// @param devUID The device UID (0-254)
    SETGSDigital(byte devUID);

    /// @brief Mark Deleted
    void Delete();

    /// @brief Set Pin1 mode.
    /// @param _PinMode Pin mode (0-3)
    void SETGSDigital::Pin1Mode(byte _PinMode);

    /// @brief Get Pin1 mode.
    /// @return byte Pin1 mode (0-3)
    byte Pin1Mode();

    /// @brief Set Pin1 number.
    /// @param _PinNo Pin number (0-63)
    void Pin1(byte _PinNo);

    /// @brief Get Pin1 number.
    /// @return byte Pin1 number (0-63)
    byte Pin1();

    /// @brief Set Pin2 mode.
    /// @param _PinMode Pin mode (0-3)
    void Pin2Mode(byte _PinMode);

    /// @brief Get Pin2 mode.
    /// @return byte Pin2 mode (0-3)
    byte Pin2Mode();

    /// @brief Set Pin2 number.
    /// @param _PinNo Pin number (0-63)
    void Pin2(byte _PinNo);

    /// @brief Get Pin2 number.
    /// @return byte Pin2 number (0-63)
    byte Pin2();
};

/// @class SETGSMath
/// @brief Math Device Storage / Settings (SETG)
class SETGSMath {
  private:
    int args[4];
    BitField bf;
    int devAddr = INONE; 
    int empty = INONE; 
  
  public:
    /// @brief Scans for existing devUID or finds a spot.
    /// @param devUID The device UID (0-254)
    SETGSMath(byte devUID);

    /// @brief Mark Deleted
    void Delete();

    /// @brief Set the first input device UID.
    /// @param _devUID The input device UID (0-63)
    void in1DevUID(byte _devUID);

    /// @brief Get the first input device UID.
    /// @return byte Input device UID (0-63)
    byte in1DevUID();

    /// @brief Set the math operator.
    /// @param _Oper Operator code (0-15, e.g., +,-,*,/,AVE)
    void Operator(byte _Oper);

    /// @brief Get the math operator.
    /// @return byte Operator code (0-15)
    byte Operator();

    /// @brief Set the second input device UID.
    /// @param _devUID The input device UID (0-63)
    void in2DevUID(byte _devUID);

    /// @brief Get the second input device UID.
    /// @return byte Input device UID (0-63)
    byte in2DevUID();
};

/// @class SETGSAnalog
/// @brief Analog Device Storage / Settings (SETG)
class SETGSAnalog {
  private: 
    int args[6];
    BitField bf; int devAddr = INONE; int empty = INONE;

  public:
    /// @brief Scans for existing devUID or finds a spot.
    /// @param devUID The device UID (0-254)
    SETGSAnalog(byte devUID);

    /// @brief Mark Deleted
    void Delete();

    /// @brief Set the multiplier numerator.
    /// @param _MultNumer Multiplier value (-32768 to 32767)
    void MultNumerator(int _MultNumer);

    /// @brief Get the multiplier numerator.
    /// @return int Multiplier value (-32768 to 32767)
    int MultNumerator();

    /// @brief Set the Pin number.
    /// @param _Pin Pin number (0-31)
    void Pin(byte _Pin);

    /// @brief Get the Pin number.
    /// @return byte Pin number (0-31)
    byte Pin();

    /// @brief Set the raw read offset.
    /// @param _ROffset Offset value (-1024 to 1024)
    void RawOffset(int _ROffset);

    /// @brief Get the raw read offset.
    /// @return int Offset value (-1024 to 1024)
    int RawOffset();

    /// @brief Set the Pin mode.
    /// @param _PinMode Pin mode (0-3, Input/Pull-up/Output/Disabled)
    void PinMode(byte _PinMode);

    /// @brief Get the Pin mode.
    /// @return byte Pin mode (0-3)
    byte PinMode();

    /// @brief Set the number of samples.
    /// @param _Samples Samples count (1,4,8,16)
    void Samples(byte _Samples);

    /// @brief Get the number of samples.
    /// @return byte Samples count (1,4,8,16)
    byte Samples(); 
};

// Output Node; Need sentianl for All Nodes in Registry + Alarm const devUID
/// @class SETGSSetpoint
/// @brief Setpoint Device Storage / Settings (SETG)
class SETGSSetpoint {
  private: 
    int args[10];
    BitField bf; int devAddr= INONE;  int empty= INONE;

  public:
    /// @brief Scans for existing devUID or finds a spot.
    /// @param devUID The device UID (0-254)
    SETGSSetpoint(byte devUID);

    /// @brief Mark Deleted
    void Delete();

    /// @brief Set the setpoint value.
    /// @param _setPoint Setpoint (-1024 to 1024)
    void Setpoint(int _setPoint);

    /// @brief Get the setpoint value.
    /// @return int Setpoint (-1024 to 1024)
    int Setpoint();

    /// @brief Set the input device UID.
    /// @param _devUID Input device UID (0-63)
    void inDevUID(byte _devUID);

    /// @brief Get the input device UID.
    /// @return byte Input device UID (0-63)
    byte inDevUID();

    /// @brief Set the input tolerance value.
    /// @param _inTol Tolerance (-1024 to 1024)
    void inTolerance(int _inTol);

    /// @brief Get the input tolerance value.
    /// @return int Tolerance (-1024 to 1024)
    int inTolerance();

    /// @brief Set the output device UID.
    /// @param _devUID Output device UID (0-63)
    void outDevUID(byte _devUID);

    /// @brief Get the Output Device UID.
    /// @return byte The stored output device UID (0-63)
    byte outDevUID();

    /// @brief Set the Timeframe Tolerance enum.
    /// @param _eTimeFT Enum value (0-31) representing the timeframe tolerance
    void TimeFrameTol(byte _eTimeFT);

    /// @brief Get the Timeframe Tolerance enum.
    /// @return byte Enum value (0-31) stored for timeframe tolerance
    byte TimeFrameTol();

    /// @brief Get the Timeframe Tolerance in milliseconds.
    /// @return uint32_t Timeframe in milliseconds corresponding to the enum
    uint32_t TimeFrameTolMS();

    /// @brief Get the Timeframe Selected in a String Literal.
    /// @return String Literal representing the Set Timeframe.
    const char* TimeFrameTolStr();

    /// @brief Set the action to perform when the input is below the setpoint.
    /// @param _belowDo Action code (On, Off, Incr++, Decr--, Alarm, IncrByOff, DecrByOff, Do-Nothing)
    void BelowSetDo(byte _belowDo);

    /// @brief Get the action configured for below setpoint.
    /// @return byte Action code (On, Off, Incr++, Decr--, Alarm, IncrByOff, DecrByOff, Do-Nothing)
    byte BelowSetDo();

    /// @brief Set the Output Node Index (0-63).
    /// @param _NodeIdx Node index to write
    void outNodeIdx(byte _NodeIdx);

    /// @brief Get the Output Node Index.
    /// @return byte Node index (0-63)
    byte outNodeIdx();

    /// @brief Set the action to perform when the input is above the setpoint.
    /// @param _aboveDo Action code (On, Off, Incr++, Decr--, Alarm, IncrByOff, DecrByOff, Do-Nothing)
    void AboveSetDo(byte _aboveDo);

    /// @brief Get the action configured for above setpoint.
    /// @return byte Action code (On, Off, Incr++, Decr--, Alarm, IncrByOff, DecrByOff, Do-Nothing)
    byte AboveSetDo();

    /// @brief Set whether the device is in Auto mode.
    /// @param _IsAuto true for Auto mode, false for OFF
    void IsAuto(bool _IsAuto);

    /// @brief Get whether the device is in Auto mode.
    /// @return bool true if Auto, false if OFF
    bool IsAuto();  
};
//_____________________________________________________________________________________________________________________