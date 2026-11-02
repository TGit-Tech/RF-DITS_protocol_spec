/// @class BitField
/// @brief Stores and retrieves user-defined bit-width values in standard Byte[] form.
/// @details
/// Usage:
///   1.  Define multiple bit-widths of desired value storage in constructor.
///       - Positive bit-widths are interpreted as unsigned values.
///       - Negative bit-widths are signed fields requiring +1 bit for the sign.
///       - Example:
///           BitField bf((int[]){8,-16,5,-11,2}, 5);
///           (Last argument specifies the number of field definitions.)
///
///           - bf(0): 8-bit unsigned value
///           - bf(1): 15-bit signed value (+1 sign bit)
///           - bf(2): 5-bit unsigned value
///           - bf(3): 10-bit signed value (+1 sign bit)
///           - bf(4): 2-bit unsigned value
///
///   2.  Load standard Bytes (bf.buff[]) of the bit data.
///       - Example:  for (int i=0; i<bf.totalBytes(); i++) { bf.buff[i] = PMEMREAD(StartAddr+i); } // Load the Bytes
///
///   3.  get/set Values.
///       - setValue(byte _FieldIdx, int _val); 
///       - int getValue(byte _FieldIdx);
///
///   4.  Save standard Bytes (bf.buff[]) of the bit data.
///       - Example: for (int i=0; i<bf.totalBytes(); i++) { PMEMWRITE(StartAddr+i, bf.buff[i]; } // Save the Bytes
///
/// NOTES:
///   - Integer signedness(+/-) interpretation is set by field definitions.
///   - Auto casting will be used for non-int C++ data-types (i.e. bool, byte)
/// LIMITS:
///   - bf.FieldDef[60]: 60 field definitions per instance.
///   - bf.buff[16]: 16 bytes per instance (128 bits maximum).
#pragma once
#include <Arduino.h>
#if __has_include("Config.h")
    #include "Config.h"
#endif
#include <TGTDB.h>

class BitField {
  private:
    byte FieldDef[60];      // Field definition[s] in bit-width where unsigned is (0-127)bits and signed is >127 ([~1]128-[~128]255) by a -127 const offset.
    byte totalByteCnt = 0;  // Total Byte count to store all fields.

  public:
    byte totalBytes() const { return totalByteCnt;}   // Number of Bytes required to store bit-fields.
    byte buff[16];                                    // Local bytes memory for bit-packing.

    BitField::BitField(const int* _FieldDefs, byte _NoOfDefs) {
        DBLAENTER((_NoOfDefs),("BitField::BitField [NoOfDefs]\n"))
        byte totalBits = 0;
        memset(FieldDef, 0, sizeof(FieldDef));  // Set all FieldDef[] to 0.
        memset(buff, 0, sizeof(buff));          // Set all buff[] bytes to 0.

        if (_NoOfDefs > sizeof(FieldDef)) _NoOfDefs = sizeof(FieldDef);

        for (int i = 0; i < _NoOfDefs; i++) {
            if (_FieldDefs[i] < 0) { 
                FieldDef[i] = 127 + abs(_FieldDefs[i]); // signed field
            } else { 
                FieldDef[i] = (byte)_FieldDefs[i];     // unsigned field
            }

            // Compute total bits using actual widths
            totalBits += (FieldDef[i] > 127) ? (FieldDef[i] - 127) : FieldDef[i];
        }

        totalByteCnt = (totalBits + 7) / 8; // round up to bytes
    }

    void setValue(byte _FieldIdx, int _val) {
      DBLAAENTER((_FieldIdx),(_val),("setValue(FieldIdx, val)\n"))
      byte offset = 0;                             // compute bit offset
      for(byte i=0; i<_FieldIdx; i++) {offset += (FieldDef[i] > 127) ? (FieldDef[i]-127) : FieldDef[i];}
      bool bSigned = FieldDef[_FieldIdx] > 127;    // Signed or Unsigned handling
      byte width = (bSigned) ? FieldDef[_FieldIdx]-127 : FieldDef[_FieldIdx];

      for(byte b=0; b<width; b++) {
        byte globalBit = offset + b;
        byte bIndex = globalBit / 8;
        byte bitPos = globalBit % 8;
        if(b==0 && bSigned) {         // Keep sign-bit MSB
          if(0x8000&_val) {bitSet(buff[bIndex], bitPos);} 
          else {bitClear(buff[bIndex], bitPos);}
          continue;
        }
        if(bitRead(_val,(width-1)-b)) {   bitSet(buff[bIndex], bitPos); }
        else                          { bitClear(buff[bIndex], bitPos); }
      }
    }

    int getValue(byte _FieldIdx) {
      byte offset = 0; int ret = 0;                // compute bit offset
      for(byte i = 0; i < _FieldIdx; i++) {offset += (FieldDef[i] > 127) ? (FieldDef[i] - 127) : FieldDef[i];}
      bool bSigned = FieldDef[_FieldIdx] > 127;    // Signed or Unsigned
      byte width = (bSigned) ? (FieldDef[_FieldIdx] - 127) : FieldDef[_FieldIdx];

      for(byte b = 0; b < width; b++) {
        byte globalBit = offset + b;
        byte bIndex = globalBit / 8;
        byte bitPos  = globalBit % 8;
        if(b==0 && bSigned) {         // Neg = sign-bit MSB + fill-in
          if(bitRead(buff[bIndex],bitPos)) {ret=0xFFFF; }
          continue;
        }
        if(bitRead(buff[bIndex],bitPos))  {  bitSet(ret,((width-1)-b));} 
        else                              {bitClear(ret,((width-1)-b));}
      }
      return ret;
    }
};