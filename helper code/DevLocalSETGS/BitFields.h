#pragma once
#include <Arduino.h>

// Define example:  BitField obj((int[]){8,2,6,2,6},5) // Last # is number of [n]
class BitField {
  private:
    byte FieldDef[60];    // bitFields definition (0-127) unsigned, (128-255) signed by const offset.      
    byte totalByteCnt = 0;
  public:
    BitField::BitField(const int* _FieldDefs, byte _NoOfDefs) {
        byte totalBits = 0;
        memset(FieldDef, 0, sizeof(FieldDef));
        memset(buff, 0, sizeof(buff));

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
    byte totalBytes() const { return totalByteCnt;}
    byte buff[16];  // local storage for bit-packing bytes.

    // Bit Packing Functions
    void ByteSet(byte _FieldIdx, byte _val) {
      if (FieldDef[_FieldIdx] > 127 ) return;   // ignore signed fields
      byte offset = 0; byte width = FieldDef[_FieldIdx];
      for(int i=0; i<_FieldIdx; i++) {offset+= (FieldDef[i] > 127) ? FieldDef[i] - 127 : FieldDef[i];}

      for(byte b=0; b<width; b++) {
        byte globalBit = offset + b;
        byte bIndex = globalBit / 8;
        byte bitPos = globalBit % 8;
        if(bitRead(_val,(width-1)-b)) {   bitSet(buff[bIndex], bitPos); }
        else                          { bitClear(buff[bIndex], bitPos); }
      }
    }

    byte ByteGet(byte _FieldIdx) {
      if(FieldDef[_FieldIdx] > 127) return 0; // ignore signed fields
      byte offset = 0; byte ret = 0; byte width = FieldDef[_FieldIdx];        // Compute bit offset
      for(int i=0; i<_FieldIdx; i++) {offset+= (FieldDef[i] > 127) ? FieldDef[i] - 127 : FieldDef[i];}

      for(byte b=0; b<width; b++) {
        byte globalBit = offset + b;
        byte bIndex = globalBit / 8;
        byte bitPos = globalBit % 8;
        if(bitRead(buff[bIndex],bitPos))  {  bitSet(ret,((width-1)-b));} 
        else                              {bitClear(ret,((width-1)-b));}
      }
      return ret;
    }

    void IntSet(byte _FieldIdx, int _val) {
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

    int IntGet(byte _FieldIdx) {
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