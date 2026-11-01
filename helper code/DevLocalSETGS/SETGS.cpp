/******************************************************************************************************************/ /**
 * @file    SETGS.cpp
 * @brief   Ref.h
 *********************************************************************************************************************/
#ifndef _SETGS_CPP
#define _SETGS_CPP

#include "SETGS.h"

// Local Device Settings (SETG) EEPROM allocation.
// Defaults are: (80)DigMath@3-bytes/ea, (16)Analog@6-bytes/ea, (20)Setpoints@8-bytes/ea = Total 496 bytes.
// At 4096-bytes - 496 = 3600 / 14b/DIT = 257 DIT Records space (about = to the 255 byte NDIT addressing).
#define DIGMATH_LSETGBYTES  3
#define ANALOG_LSETGBYTES   6
#define SETPOINT_LSETGBYTES 8

#define DIGMATH_MAXSETGS    80
#define ANALOG_MAXSETGS     16
#define SETPOINT_MAXSETGS   20

#define DIGMATH_STARTADDR   0
#define ANALOG_STARTADDR (DIGMATH_STARTADDR + (DIGMATH_MAXSETGS * DIGMATH_LSETGBYTES))
#define SETPOINT_STARTADDR (ANALOG_STARTADDR + (ANALOG_MAXSETGS * ANALOG_LSETGBYTES))
#define SETG_ENDADDR (SETPOINT_STARTADDR + (SETPOINT_MAXSETGS * SETPOINT_LSETGBYTES))

#define DIGMATH_IDXADDR(idx)   (DIGMATH_STARTADDR + (idx * DIGMATH_LSETGBYTES))
#define ANALOG_IDXADDR(idx)    (ANALOG_STARTADDR + (idx * ANALOG_LSETGBYTES))
#define SETPOINT_IDXADDR(idx)  (SETPOINT_STARTADDR + (idx * SETPOINT_LSETGBYTES))

// Comment-out for actual usage
byte RAMMem[SETG_ENDADDR];
void RAMWrite(byte _Addr, byte _Val) {RAMMem[_Addr] = _Val;}
byte RAMRead(uint16_t _Addr) {return RAMMem[_Addr];}
void InitRAMMem() {memset(RAMMem, 0xFF, sizeof(RAMMem));}

#define PMEMWRITE      RAMWrite
#define PMEMREAD       RAMRead

// -------------------------------------------------------------------------------------------------
// bf 24-bits (3-bytes) per Digital Device. (sonar/serial are 2-pin)
//(0) 8-bits  0-254  devUID
//(1) 2-bits  0-3   Pin1 PinMode
//(2) 6-bits  0-63  Pin1
//(3) 2-bits  0-3   Pin2 PinMode
//(4) 6-bits  0-63  Pin2
SETGSDigital::SETGSDigital(byte devUID) : args{8,2,6,2,6}, bf(args,5) {
  for (int adr=DIGMATH_STARTADDR; adr<ANALOG_STARTADDR ; adr=adr+DIGMATH_LSETGBYTES) { 
    byte rbyte = PMEMREAD(adr);
    if (empty==INONE && rbyte==BNONE) { empty = adr; }
    if (rbyte==devUID) {devAddr = adr; break; }
  }
  if (devAddr==INONE && empty!=INONE) { devAddr=empty; PMEMWRITE(devAddr,devUID); }
  if (devAddr==INONE) { Serial.println("No DigMath Storage left."); return; }
  for (int i=0; i<bf.totalBytes(); i++) { bf.buff[i] = PMEMREAD(devAddr+i); } // Load the Bytes
  Serial.print("devAddr: ");Serial.println(devAddr);
}
void SETGSDigital::Delete() { bf.ByteSet(0,BNONE); }
void SETGSDigital::Pin1Mode(byte _PinMode) { bf.ByteSet(1,_PinMode); }
byte SETGSDigital::Pin1Mode() { return bf.ByteGet(1); }
void SETGSDigital::Pin1(byte _PinNo) {bf.ByteSet(2, _PinNo);}
byte SETGSDigital::Pin1() {return bf.ByteGet(2);}
void SETGSDigital::Pin2Mode(byte _PinMode) {bf.ByteSet(3, _PinMode);}
byte SETGSDigital::Pin2Mode() {return bf.ByteGet(3);}
void SETGSDigital::Pin2(byte _PinNo) {bf.ByteSet(4, _PinNo);}
byte SETGSDigital::Pin2() {return bf.ByteGet(4);}
// -------------------------------------------------------------------------------------------------
// bf 24-bits (3-bytes) per Math Device.
//(0) 8-bits  0-254   devUID
//(1) 6-bits  0-63    Input devUID1
//(2) 4-bits          Math Operator (+,-,*,/,AVE)
//(3) 6-bits  0-63    Input devUID2
SETGSMath::SETGSMath(byte devUID) : args{8,6,4,6}, bf(args,4) {
  for (int adr=DIGMATH_STARTADDR; adr<ANALOG_STARTADDR ; adr=adr+DIGMATH_LSETGBYTES) { 
    byte rbyte = PMEMREAD(adr);
    if (empty==INONE && rbyte==BNONE) { empty = adr; }
    if (rbyte==devUID) {devAddr = adr; break; }
  }
  if (devAddr==INONE && empty!=INONE) { devAddr=empty; PMEMWRITE(devAddr,devUID); }
  if (devAddr==INONE) { Serial.println("No DigMath Storage left."); return; }
  for (int i=0; i<bf.totalBytes(); i++) { bf.buff[i] = PMEMREAD(devAddr+i); } // Load the Bytes
  Serial.print("devAddr: ");Serial.println(devAddr);
}
void SETGSMath::Delete() { bf.ByteSet(0,BNONE); }
void SETGSMath::in1DevUID(byte _devUID) { bf.ByteSet(1,_devUID);}
byte SETGSMath::in1DevUID() { return bf.ByteGet(1); }
void SETGSMath::Operator(byte _Oper) { bf.ByteSet(2,_Oper); }
byte SETGSMath::Operator() { return bf.ByteGet(2); }
void SETGSMath::in2DevUID(byte _devUID) { bf.ByteSet(3,_devUID); }
byte SETGSMath::in2DevUID() { return bf.ByteGet(3); }
// -------------------------------------------------------------------------------------------------
// bf   44-bits (6-bytes); per Analog Device.
// (0)  8-bits    devUID
// (1)  16-bits   +-32K Signed Integer (Multiplier/const-1000).
// (2)  5-bits    0-31  Pin ID
// (3)  11-bits   +-1024  Raw-Read Offset
// (4)  2-bit     0-3   Pin Mode
// (5)  2-bit     Samples (1,4,8,16)
SETGSAnalog::SETGSAnalog(byte devUID) : args{8,-16,5,-11,2,2}, bf(args,6) {
  for (int adr=ANALOG_STARTADDR; adr<SETPOINT_STARTADDR ; adr=adr+ANALOG_LSETGBYTES) { 
    byte rbyte = PMEMREAD(adr);
    if (empty==INONE && rbyte==BNONE) { empty = adr; }
    if (rbyte==devUID) {devAddr = adr; break; }
  }
  if (devAddr==INONE && empty!=INONE) { devAddr=empty; PMEMWRITE(devAddr,devUID); }
  if (devAddr==INONE) { Serial.println("No DigMath Storage left."); return; }
  for (int i=0; i<bf.totalBytes(); i++) { bf.buff[i] = PMEMREAD(devAddr+i); } // Load the Bytes
  Serial.print("devAddr: ");Serial.println(devAddr);
}
void SETGSAnalog::Delete() {bf.ByteSet(0, BNONE);}
void SETGSAnalog::MultNumerator(int _MultNumer) { bf.IntSet(1, _MultNumer); }
int SETGSAnalog::MultNumerator() { return bf.IntGet(1); }
void SETGSAnalog::Pin(byte _Pin) { bf.ByteSet(2, _Pin); }
byte SETGSAnalog::Pin() { return bf.ByteGet(2); }
void SETGSAnalog::RawOffset(int _ROffset) { bf.IntSet(3, _ROffset); }
int SETGSAnalog::RawOffset() { return bf.IntGet(3); }
void SETGSAnalog::PinMode(byte _PinMode) { bf.ByteSet(4, _PinMode); }
byte SETGSAnalog::PinMode() { return bf.ByteGet(4); }
void SETGSAnalog::Samples(byte _Samples) { bf.ByteSet(5, _Samples); }
byte SETGSAnalog::Samples() { return bf.ByteGet(5); }
// -------------------------------------------------------------------------------------------------
// bf 61-bits (8-bytes) per Setpoint Device.
// (0)8-bits    0-255   devUID
// (1)6-bits    0-63    Input devUID
// (2)11-bits   +-1024  The Setpoint
// (3)11-bits   +-1024  Input Tolerance Value
// (4)5-bits    0-31    Enum Timeframe Tolerance
// (5)1-bit     0-1     Status
// (6)6-bits    0-63    Output NodeIdx
// (7)6-bits    0-63    Output devUID
// (8)3-bits    0-7     Below Setpoint do (On, Off, Incr++, Decr--, Alarm, IncrByOff, DecrByOff, Do-Nothing)
// (9)3-bits    0-7     Above Setpoint do (On, Off, Incr++, Decr--, Alarm, IncrByOff, DecrByOff, Do-Nothing)
SETGSSetpoint::SETGSSetpoint(byte devUID) : args{8,6,-11,-11,5,1,6,6,3,3}, bf(args,10) {
  for (int adr=SETPOINT_STARTADDR; adr<SETG_ENDADDR ; adr=adr+SETPOINT_LSETGBYTES) { 
    byte rbyte = PMEMREAD(adr);
    if (empty==INONE && rbyte==BNONE) { empty = adr; }
    if (rbyte==devUID) {devAddr = adr; break; }
  }
  if (devAddr==INONE && empty!=INONE) { devAddr=empty; PMEMWRITE(devAddr,devUID); }
  if (devAddr==INONE) { Serial.println("No DigMath Storage left."); return; }
  for (int i=0; i<bf.totalBytes(); i++) { bf.buff[i] = PMEMREAD(devAddr+i); } // Load the Bytes
  Serial.print("devAddr: ");Serial.println(devAddr);
}
void SETGSSetpoint::Delete() { bf.ByteSet(0, BNONE);}
void SETGSSetpoint::inDevUID(byte _devUID) { bf.ByteSet(1,_devUID); }
byte SETGSSetpoint::inDevUID() { return bf.ByteGet(1); }
void SETGSSetpoint::Setpoint(int _setPoint) { bf.IntSet(2, _setPoint); }
int SETGSSetpoint::Setpoint() { return bf.IntGet(2); }
void SETGSSetpoint::inTolerance(int _inTol) { bf.IntSet(3,_inTol); }
int SETGSSetpoint::inTolerance() { return bf.IntGet(3); }
void SETGSSetpoint::TimeFrameTol(byte _eTimeFT) { bf.ByteSet(4,_eTimeFT); }
byte SETGSSetpoint::TimeFrameTol() { return bf.ByteGet(4); }
void SETGSSetpoint::IsAuto(bool _IsAuto) { bf.ByteSet(5,_IsAuto?1:0); }
bool SETGSSetpoint::IsAuto() { return bf.ByteGet(5) != 0; }
void SETGSSetpoint::outNodeIdx(byte _NodeIdx) { bf.ByteSet(6,_NodeIdx); }
byte SETGSSetpoint::outNodeIdx() { return bf.ByteGet(6); }
void SETGSSetpoint::outDevUID(byte _devUID) { bf.ByteSet(7,_devUID); }
byte SETGSSetpoint::outDevUID() { return bf.ByteGet(7); }
void SETGSSetpoint::BelowSetDo(byte _belowDo) { bf.ByteSet(8,_belowDo); }
byte SETGSSetpoint::BelowSetDo() { return bf.ByteGet(8); }
void SETGSSetpoint::AboveSetDo(byte _aboveDo) { bf.ByteSet(9,_aboveDo); }
byte SETGSSetpoint::AboveSetDo() { return bf.ByteGet(9); }

uint32_t SETGSSetpoint::TimeFrameTolMS() {
  if (devAddr==INONE) return 0xFFFFFFFF;
  byte enumNo = bf.ByteGet(4);;
  static const uint16_t TimeframeTolerance[32] PROGMEM = {
    43200, 37800, 32400, 27000, 21600,
    18900, 16200, 13500, 10800,
    9000, 7200, 5400, 3600,
    3000, 2400, 1800, 1200,
    930, 660, 390, 120,
    95, 70, 45, 20,
    15, 10, 5,
    3, 2, 1, 0 };
  return uint32_t(pgm_read_word(&TimeframeTolerance[enumNo])) * 2000;
}

const char* SETGSSetpoint::TimeFrameTolStr() {
  switch(bf.ByteGet(4)) {
    case 0: return "24h";   case 1: return "21h";   case 2: return "18h";   case 3: return "15h";   case 4: return "12h";
    case 5: return "10h30"; case 6: return "9h";    case 7: return "7h30";  case 8: return "6h";    case 9: return "5h15";
    case 10: return "4h30"; case 11: return "3h45"; case 12: return "3h";   case 13: return "2h30"; case 14: return "2h";
    case 15: return "1h30"; case 16: return "1h";   case 17: return "60m";  case 18: return "45m";  case 19: return "30m";
    case 20: return "15m";  case 21: return "12m";  case 22: return "10m";  case 23: return "7m";   case 24: return "5m";
    case 25: return "3m";   case 26: return "2m";   case 27: return "1m";   case 28: return "59s";  case 29: return "45s";
    case 30: return "30s";  case 31: return "1s";   default: return "";
  }
}
//_____________________________________________________________________________________________________________________
#endif