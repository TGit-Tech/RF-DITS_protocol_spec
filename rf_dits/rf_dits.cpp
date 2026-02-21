/******************************************************************************************************************/ /**
 * @file    rf_dits.cpp
 * @brief   Ref.h
 *********************************************************************************************************************/
#ifndef _RF_DITS_CPP
#define _RF_DITS_CPP
#define CHALLENGE_WINDOW_MS 10000UL

#include "rf_dits.h"
// -------------------------------------------------------------------------------------------------
// TOC
// DITSPUBLOC 1. DITS Public Local Management.
// DITSPUBREM 2. DITS Public Remote Management.
// DITSPRVLOC 3. DITS Private Local Management.
// DITSPRVREM 4. DITS Private Remote Management.
// -------------------------------------------------------------------------------------------------
static bool s_rngSeeded = false;               // RF-DITS RNG Seed (static to this translation unit)
static uint16_t s_challengeNonce = 0;          // Static global to store nonce on each node (Target)
static unsigned long s_challengeIssuedMS = 0;  // Used to expire issued SetKey
static void RFDITS_SeedRNG() {
  if (s_rngSeeded) return;
  uint32_t s = micros();
  uint32_t x;
  s ^= (uint32_t)&x;
  randomSeed(s);
  s_rngSeeded = true;
}
//########################## 1. DITS Public Local Management.########################################
DITSEngine::DITSEngine(uint16_t _pmemBeginAddr, uint8_t _MaxDITRecords, uint8_t _NameFieldBytes)
    : pmSecNetAddr(_pmemBeginAddr), pmDITbase(_pmemBeginAddr+1), pmMaxDITRecords(_MaxDITRecords),
      pmDITend((_pmemBeginAddr+1)+(_MaxDITRecords*(PMO_NAME+_NameFieldBytes))), pmDITEndAddr(pmDITend+(PMO_NAME+_NameFieldBytes)),
      DITNameBytes(_NameFieldBytes), DITNameChar(_NameFieldBytes+1)
  {
    DBDITAAAENTER((_pmemBeginAddr),(_MaxDITRecords),(_NameFieldBytes),("DITSEngine::DITSEngine(<BeginAddr>,<MaxDITS>,<NameBytes>)\n"))
    DBDITAENTER((pmSecNetAddr), ("pmSecNetAddr\n"))
    DBDITAENTER((pmDITbase),    ("pmDITbase\n"))
    DBDITAENTER((pmDITend),     ("pmDITend\n"))
    DBDITAENTER((pmDITEndAddr), ("pmDITEndAddr\n"))
    DBDITAENTER((pmDITEndAddr-pmSecNetAddr),("<< Total Bytes Allocated for DIT Records.")) 

    // Scan pmem alloc ends to center for PMDITSTOP marker.  Make sure one exists on both NDIT & DDIT.
    ScanPmemForDITSTOP();
}
//-------------------------------------------------------------------------------------------------
uint8_t DITSEngine::SecNetCode() const { return pmem_read(pmSecNetAddr); }
//-------------------------------------------------------------------------------------------------
bool DITSEngine::SecNetCode(uint8_t _SecNetCode) {
  if (_SecNetCode<=SECNETMAX) { pmem_write(pmSecNetAddr, _SecNetCode); return true; }
  return false;
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::UpdateThisNode(uint16_t RFAddr, const char* name) {
  NodeDIT node(this,0);
  if (node.NodeIdx() == BNONE) {
    DBERROR(("DITSEngine::UpdateThisNode node.NodeIdx() == BNONE"))
    return;
  }
  node.NRFAddrH(highByte(RFAddr));
  node.NRFAddrL(lowByte(RFAddr));
  node.NSetName(name);
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::AddThisNodeDevice(uint8_t devType, const char* name) {
  if (!name) {DBDITERROR(("DITSEngine::AddThisNodeDevice !name.")) return false;}
  DBDITAAENTER((devType),(name),("DITSEngine::AddThisNodeDevice(<devNodeIdx>,devType,<name>)"))
  
  uint32_t devUIDMask[(DDITStopIdx + 31) / 32];   // Dynamically created a bitmask array for used devUIDs
  memset(devUIDMask, 0, sizeof(devUIDMask));      // Set all bytes in the array to 0
  
  uint16_t freeDITidx = BNONE;
  for (DeviceDIT dev(this); dev.IsValid(); dev.Next()) {
    if(dev.IsDeleted()) {if (freeDITidx==BNONE) {freeDITidx = dev.DITidx();}  continue;}
    if(dev.DNodeIdx()==0) {
      uint8_t thisUID = dev.DevUID();
      bitSet(devUIDMask[thisUID/32], thisUID%32);
    }
  }
  //------------------------------------------------------
  uint8_t devUID = BNONE;
  for (uint8_t UIDMaskIdx = 0; UIDMaskIdx < (DDITStopIdx + 31) / 32; ++UIDMaskIdx) {
    for (uint8_t bitIdx = 0; bitIdx < 32; ++bitIdx) {     // Iterate through each bit
      if (!bitRead(devUIDMask[UIDMaskIdx], bitIdx)) {     // Check if the bit is not set
        devUID = UIDMaskIdx * 32 + bitIdx; break;         // break; Found the first available DevUID
      }
    }
    if (devUID != BNONE) { break; }
  }
  if (devUID == BNONE) {DBDITERROR(("DITSEngine::AddThisNodeDevice devUID==BNONE.\n")) return false;}
  //------------------------------------------------------
  if (freeDITidx == BNONE) { freeDITidx = AddDDIT(); }
  DeviceDIT dev(freeDITidx);    // Now, create the DeviceDIT struct and assign the values
  dev.DNodeIdx(0);              // Associate locally with "this node"(0)
  dev.DevUID(devUID);           // Assign the DevUID found above
  dev.DevType(devType);         // Assign devType
  dev.DSetName(name);           // Assign the name
  return true;                  // Return success
} 
//-------------------------------------------------------------------------------------------------
bool DITSEngine::DelThisNodeDevice(uint8_t _devUID) {
  for(DeviceDIT dev(this); dev.IsValid(); dev.Next()) {
    if (dev.DNodeIdx()!=0) continue;
    if (dev.DevUID()==_devUID) {dev.IsDeleted(true);return true;}
  }
  DBAINFO((_devUID),("DITSEngine::DelThisNodeDevice(<devUID>)\n"))
  return false;  // Return false if the device not found.
}
//########################## 2. DITS Public Remote Management.########################################
void DITSEngine::ProcessLoop() {
  if (rxPacket && rxPacket->IsComplete(millis())) {  // If the packet is non-null and complete (either finished or expired)
    if (rxPacket->IsValid()) RxProcessPacket();      // Process, if packet is valid (not-expired or unauthorized)
    delete rxPacket;
    rxPacket = nullptr;  // Delete the Packet
  }
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::RxData(uint8_t _uint8_t) {
  //RxPacket(uint8_t _RadioPktMaxBytes, uint8_t _SecNet, uint32_t _expireMillis = 30000);
  if (!rxPacket) rxPacket = new RxPacket(mRadioPktMaxBytes, SecNetCode());
  rxPacket->RxByte(_uint8_t);
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::RadioPktMaxBytes(uint16_t maxBytes) {
  if (maxBytes >= PKC_RADIOPKTMAXBYTES_MIN && maxBytes <= PKC_RADIOPKTMAXBYTES_MAX) {
    mRadioPktMaxBytes = maxBytes; return true;}  // Success
  DBRFAERROR((maxBytes),("DITSEngine::RadioPktMaxBytes(<maxBytes>) is out of range.\n"))
  return false;  // Failure
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::TxAddRemoteNode(int RFAddr) {
  if (RFAddr == 0) return;

  // Build a PKT_REQDEVITBL packet to the target RF address
  //TxPacket pkt(SecNetCode, PKT_REQDEVITBL, RFAddr, NodeVer);
  //Send(&pkt);  // Implementer handles actual radio transmission

  DBRFAINFO((RFAddr, HEX),("DITSEngine::TxAddRemoteNode PKT_REQDEVITBL sent to <RFAddr>\n"));
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::TxSendPacket(TxPacket* Tx) {
  /*
  DBRFAENTER((int(Tx),HEX),("****************Euint8_t::Send(<Tx>)*********************\n"))
  int i=0;
  
  // Prep & Check
  if ( Tx == 0 ) return;
  if ( EParamGotBytes==0 ) GetParam();                              // Get Parameters
  if ( Mode() != ERFNORMMODE ) { DBERRORAL(("Euint8_t::Send(Tx) Radio in Mode : "),(Mode())) return; }


  
  // Set FromRF
  unsigned int FromRF = Address();
  if ( FromRF == 0xFFFF ) { DBERRORL(("Euint8_t::Send(Tx) FromRF=0xFFFF")) return; }
  Tx->Bytes[PKB_FROM_RFH] = highByte(FromRF);
  Tx->Bytes[PKB_FROM_RFL] = lowByte(FromRF);
  
  // Set Channel & Secure
  Tx->Bytes[PKB_CHANNEL] = Channel();
  Tx->Secure();

  // Re-Pack Bytes into Send Queue and show DEBUG INFO
  ArduinoQueue<uint8_t> bSendQueue(518); 
  DBINFOAAL(("To[0][1]: "),(Tx->Bytes[PKB_TO_RFH],HEX),(Tx->Bytes[PKB_TO_RFL],HEX))
  DBINFOAL(("Channel[2]: "),(Tx->Bytes[PKB_CHANNEL],HEX))
  DBINFOAAL(("SecNet,Size: "),(EEPROM.read(EMC_SECNET)),(Tx->Size))
  DBINFOAAL(("SecNet,Size[3][4]: "),(Tx->Bytes[PKB_SECH],HEX),(Tx->Bytes[PKB_SECL],HEX))
  DBINFOAL(("HSize+PacketType[5]: "),(Tx->Bytes[PKB_TYPE],HEX))
  DBINFOAAL(("From[6][7]: "),(Tx->Bytes[PKB_FROM_RFH],HEX),(Tx->Bytes[PKB_FROM_RFL],HEX))
  DBINFOAL(("Tode Version[8]: "),(Tx->Bytes[PKB_TODEVER],HEX))
  //#define PKB_TYPE        5     // This point forward may differ
  //#define PKB_FROM_RFH    6
  //#define PKB_FROM_RFL    7
  //#define PKB_TODEVER     8
  //#define PKB_TODECONFIG  9     // Start Tode Config Data
  //#define PKB_RFID        9     // First RFID on GETVALS
  //#define PKB_VALUEH      10
  //#define PKB_VALUEL      11
  
  i=0; uint8_t Temp=0;
  while ( i<Tx->Size ) {
    if ( i%12 == 0 ) { DBENTERL((" ")) } // new line
    if ( i<58 ) { 
      bSendQueue.enqueue(Tx->Bytes[i]);      
      DBENTERAA(("Tx"),(i),(Tx->Bytes[i],HEX))
    } else {
      if ( Tx->ExtraBytes == 0 ) { DBERRORAL(("Euint8_t::Send Tx->ExtraBytes == 0 @i: "),(i)) break; }
      else {
        if ( Tx->ExtraBytes->isEmpty() ) { DBINFOL(("Euint8_t::Send Tx-ExtraBytes->isEmpty()")) break; }
        Temp = Tx->ExtraBytes->dequeue();
        DBENTERAA(("Ex"),(i),(Temp))
        bSendQueue.enqueue(Temp);
      }
    }
    i++;
  }
  DBENTERL((""))

  // Send the Send-Queue
  i=0; 
  while ( !bSendQueue.isEmpty() ) { 
    i++;
    if (i==58) {
      RFSERIAL.flush();
      while ( digitalRead(PinAUX) == LOW ) { };     // Wait for not busy to send another 512-uint8_ts
      RFSERIAL.write(Tx->Bytes[PKB_TO_RFH]);         // Resend Header and continue
      RFSERIAL.write(Tx->Bytes[PKB_TO_RFL]);
      RFSERIAL.write(Tx->Bytes[PKB_CHANNEL]);   
    }
    RFSERIAL.write(bSendQueue.dequeue());
  }  
  */
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::DelRemoteNode(uint8_t nodeIdx) {
  NodeDIT node(this, nodeIdx); node.IsDeleted(true);
  for(DeviceDIT dev(this); dev.IsValid(); dev.Next()) {if (dev.DNodeIdx()==nodeIdx) dev.IsDeleted(true);}
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::TxSetRemoteDevVal(uint16_t nodeAddr, uint8_t ditIdx, int value) {
  //TxPacket pkt(SecNetCode, PKT_VAL, nodeAddr, NodeVer, ditIdx, value);
  //Send(&pkt);  // Engine handles transport internally
}
//########################## 3. DITS Private Local Management.########################################
// TODO:  Use struct to iterate not PMEM
void DITSEngine::ScanPmemForDITSTOP() {
  bool StopFound = false;
  for (uint8_t idx=0; idx<pmMaxDITRecords; ++idx) {
    if (pmem_read(pmNDITAddr(idx))==PMDITSTOP) {NDITStopIdx=idx; break;}
  }
  if (NDITStopIdx==0) {pmem_write(pmNDITAddr(0), PMDITSTOP);}
  for (uint8_t idx=0; idx<pmMaxDITRecords; ++idx) {
    if (pmem_read(pmDDITAddr(idx))==PMDITSTOP) {DDITStopIdx=idx; break;}
  }
  if (DDITStopIdx==0) {pmem_write(pmDDITAddr(0), PMDITSTOP);}
}
//-----------------------------------------------------------------------------------------------------
int8_t DITSEngine::FindNodeByRF(uint16_t rfAddr) {
  uint8_t rfLow = rfAddr & 0xFF;          // extract low uint8_t
  uint8_t rfHigh = (rfAddr >> 8) & 0xFF;  // extract high uint8_t
  for(NodeDIT node(this); node.IsValid(); node.Next()) {
    if(node.IsDeleted()) continue;
    if(node.NRFAddrL()!=rfLow) continue;
    if(node.NRFAddrH()!=rfHigh) continue;
    return node.DITidx();
  }
  return BNONE;
}
//-------------------------------------------------------------------------------------------------
int8_t DITSEngine::AddNDIT() {
  if ( DDITStopIdx+NDITStopIdx+2 > pmMaxDITRecords) {   // Check DIT Record boundary
    DBDITERROR(("DITSEngine::AddNDIT DDITStopIdx+NDITStopIdx+2>pmMaxDITRecords Out of Memory.\n"))
    return BNONE;
  }
  NDITStopIdx++;
  pmem_write(pmNDITAddr(NDITStopIdx) + PMO_NODEIDX, PMDITSTOP);  // write direct, avoid IsValid trap.
  return NDITStopIdx - 1;
}
//-------------------------------------------------------------------------------------------------
int8_t DITSEngine::AddDDIT() {
  if ( DDITStopIdx+NDITStopIdx+2 > pmMaxDITRecords) {   // Check DIT Record boundary
    DBDITERROR(("DITSEngine::AddDDIT DDITStopIdx+NDITStopIdx+2>pmMaxDITRecords Out of Memory.\n"))
    return BNONE;
  }
  DDITStopIdx++;                                                  // Increment.
  pmem_write(pmDDITAddr(DDITStopIdx) + PMO_DNODEIDX, PMDITSTOP);  // write direct, avoid IsValid trap.
  return DDITStopIdx - 1;
}
//########################## 4. DITS Private Remote Management.########################################CHK
DITSEngine::RxPacket::RxPacket(uint16_t _RadioPktMaxBytes, uint8_t _SecNet, uint32_t _expireMillis) {
  DBRFAAENTER((_RadioPktMaxBytes),(_expireMillis),("RxPacket::RxPacket(<RadioPktMaxBytes>,SecNet,<expireMillis>)"))
  uiRadioPktMaxBytes = _RadioPktMaxBytes;
  rxPoolIdxs = (512 + (uiRadioPktMaxBytes - 1)) / uiRadioPktMaxBytes;  //512-byte limit on SecNet encoding
  if (rxPoolIdxs <= 0 ) {DBRFAERROR((_RadioPktMaxBytes),("RxPacket::RxPacket RadioPktMaxBytes/512 <= 0")) while(1);}
  rxPool = new uint8_t*[rxPoolIdxs];
  for (int i=0; i<rxPoolIdxs; i++) {rxPool[i]=nullptr;} // Initialize every i in array to nullptr    
  SecNet = _SecNet; PcktExprMS = _expireMillis; PcktBegMS = millis();
}
//-----------------------------------------------------------------------------------------------------CHK
DITSEngine::RxPacket::~RxPacket() {
  DBRFENTER(("RxPacket::~RxPacket()"))
  for (int i = 0; i<rxPoolIdxs; i++) {if (rxPool[i]) {delete[] rxPool[i];rxPool[i] = nullptr;}}
  delete[] rxPool; rxPool = nullptr;
} // destruct rxPool properly
//-----------------------------------------------------------------------------------------------------CHK
void DITSEngine::RxPacket::RxByte(uint8_t _Byte) {
  uint16_t pi = NextIdx/uiRadioPktMaxBytes; uint16_t i = NextIdx-(pi*uiRadioPktMaxBytes);
  DBRFAAAENTER((pi),(i),(_Byte,HEX),("RxPacket::RxByte rxPool[<pi>][<i>]=<uint8_t>\n"))
  if (!rxPool[pi]) {rxPool[pi] = new uint8_t[uiRadioPktMaxBytes];}
  rxPool[pi][i] = _Byte;                                          // assign Byte in rxPool
  NextIdx++;                                                      // incr counter
  if (NextIdx == 5) {if (!IsSecure()) {PcktBegMS = 0; return;}}   // End on Insecure uint8_t.
  if (NextIdx > 5 && NextIdx >= Size) {PcktBegMS = 0;}            // Flag Packet Done when Size is reached.
  return;
}
//-----------------------------------------------------------------------------------------------------CHK
bool DITSEngine::RxPacket::IsSecure() {
  DBRFENTER(("RxPacket::IsSecure")) // Check - Good for 512b, 128-Combo SecNet, Add nonce bypass
  if (!rxPool[0]) return false;
  int SecNet = word(rxPool[0][PKB_SECH], rxPool[0][PKB_SECL]);  //0,1
  uint8_t Sc = 0;uint16_t Sz = 0;int i = 0;int y = 0;

  while (i < 14) {
    bitWrite(Sc, y, bitRead(SecNet, i));i++;
    bitWrite(Sz, y, bitRead(SecNet, i));i++;
    y++;
  }
  bitWrite(Sz, y, bitRead(SecNet, i));y++;i++;
  bitWrite(Sz, y, bitRead(SecNet, i));
  DBRFAINFO((Sc, HEX),("RxPacket::IsSecure <Sc>\n"))
  DBRFAINFO((Sz),("RxPacket::IsSecure <Sz>\n"))
  if (Sc != SecNet) {DBRFERROR(("RxPacket::IsSecure FAILED SECURITY")) return false;}
  Size = Sz;
  // ---- Check Nonce on SETVAL ----
  /* skip for now.  test later.
  if (Type() == PKT_SETVAL) {
    uint16_t receivedNonce = (Bytes[12] << 8) | Bytes[13];
    unsigned long nowMS = millis();
    if (receivedNonce != s_challengeNonce) { 
      DBRFERROR(("RxPacket::IsSecure SETVAL nonce mismatch"))
      return false;
    }
    if (nowMS - s_challengeIssuedMS > CHALLENGE_WINDOW_MS) { 
      DBRFERROR(("RxPacket::IsSecure SETVAL nonce expired"))
      s_challengeNonce = 0; return false;
    }
  }
  */
  return true;
}
//-----------------------------------------------------------------------------------------------------TxPacket
//-----------------------------------------------------------------------------------------------------
DITSEngine::TxPacket::TxPacket(uint8_t _RadioPktMaxBytes, uint8_t _SecNet, uint8_t _Type, int _ToRF, uint8_t _Ver, uint8_t _DevUID, int _Value) {
  
  DBRFAAAENTER((_Type,HEX),(_ToRF,HEX),(_DevUID),("TxPacket::TxPacket(MaxRB,SecNet,Ver,<Type>,<ToRF>,<DevUID>,Value)"))
  // Setup txPool
  txPoolIdxs = (512 + (_RadioPktMaxBytes - 1)) / _RadioPktMaxBytes;  //512-uint8_t limit on SecNet encoding
  if (txPoolIdxs <= 0 ) { DBRFAERROR((_RadioPktMaxBytes),("TxPacket::TxPacket RadioPktMaxBytes/512 <= 0")) while(1);}
  txPool = new uint8_t*[txPoolIdxs];
  for (int i=0; i<txPoolIdxs; i++) {txPool[i]=nullptr;} // Initialize every i in array to nullptr
  txPool[0] = new uint8_t[uiRadioPktMaxBytes];               // Initialze first set of uint8_t

  // Packet-Type to Byte-Size ( Size is Byte[i]+1 )
  if (_Type == PKT_REQDEVITBL) {
    DBRFINFO(("TxPacket::TxPacket PKT_REQDEVITBL\n"))
    Size = 8;
  } else if (_Type == PKT_REQVALS) {
    DBRFINFO(("TxPacket::TxPacket PKT_REQVALS\n"))
    Size = 9;
  } else if (_Type == PKT_REQVAL) {
    DBRFINFO(("TxPacket::TxPacket PKT_REQVAL\n"))
    Size = 10;
  } else if (_Type == PKT_SETVAL) {
    DBRFINFO(("TxPacket::TxPacket PKT_SETVAL\n"))
    Size = 12 + 2;
    //uint16_t nonce = _SetKey ^ _SecNet;
    //Bytes[12] = (nonce >> 8) & 0xFF;
    //Bytes[13] = nonce & 0xFF;
  } else if (_Type == PKT_VAL) {
    DBRFINFO(("TxPacket::TxPacket PKT_VAL\n"))
    Size = 12;
  }

  else if (_Type == PKT_DEVITBL) {
    DBRFINFO(("TxPacket::TxPacket PKT_DEVITBL\n"))
    Size = PKB_DEVITBL;
  }  // Start TodeConfig at Byte[9]
  else if (_Type == PKT_VALS) {
    DBRFINFO(("TxPacket::TxPacket PKT_VALS\n"))
    Size = PKB_DEVUID;
  }  // Start Values at Byte[9]
  else if (_Type == PKT_RSPSETKEY) {
    Size = 12;  // standard RSPSETVAL size

    // ---- Generate Nonce ----
    RFDITS_SeedRNG();                                              // ensure random seeded
    s_challengeNonce = (random(0, 0xFFFF) ^ (millis() & 0xFFFF));  // 16-bit nonce
    s_challengeIssuedMS = millis();                                // timestamp

    // Send Key back for verification
    _Value = s_challengeNonce;
    // XOR with SecNet for verification
    s_challengeNonce ^= _SecNet;
  } else {
    DBRFAERROR((_Type, HEX),("TxPacket::TxPacket Unidentified Packet (Type).\n"))
    return;
  }

  txPool[0][PKB_TO_RFH] = highByte(_ToRF);  //0
  txPool[0][PKB_TO_RFL] = lowByte(_ToRF);   //1
  //Bytes[PKB_CHANNEL] set in Send()      //2   Set in E32::Send()
  //Bytes[PKB_SECH] = highByte(SecNet);   //3   Set in Secure()
  //Bytes[PKB_SECL] = lowByte(SecNet);    //4   Set in Secure()
  txPool[0][PKB_TYPE] = _Type;  //5
  //Bytes[PKB_FROM_RFH] set in Send()     //6   Set in E32::Send()
  //Bytes[PKB_FROM_RFL] set in Send()     //7   Set in E32::Send()
  txPool[0][PKB_DITVER] = _Ver;  //8

  // ------ End of Static Sets ----------
  txPool[0][PKB_DEVUID] = _DevUID;        //9
  txPool[0][PKB_VALUEH] = highByte(_Value);  //10
  txPool[0][PKB_VALUEL] = lowByte(_Value);   //11
}
//-----------------------------------------------------------------------------------------------------
DITSEngine::TxPacket::~TxPacket() {
  DBRFENTER(("TxPacket::~TxPacket()"))
  for (int i = 0; i<txPoolIdxs; i++) {if (txPool[i]) {delete[] txPool[i];txPool[i] = nullptr;}}
  delete[] txPool; txPool = nullptr;
}
//-----------------------------------------------------------------------------------------------------
void DITSEngine::TxPacket::TxByte(uint8_t _Byte) {
/*
  if (Size < PKC_STATICBYTES) {
    Bytes[Size] = _Byte;
  } else {
    if (ExtraBytes == 0) ExtraBytes = new ArduinoQueue<uint8_t>(512);
    if (ExtraBytes->isFull()) {
      DBERROR(("TxPacket::TxByte ExtraBytes->isFull()"))
      return;
    }
    ExtraBytes->enqueue(_Byte);
  }
  DBAAINFO(("TxPacket::TxByte(Byte)(Idx/Size): "), (_Byte, HEX), (Size))
  Size++;
  */
}
//-----------------------------------------------------------------------------------------------------good
int DITSEngine::TxPacket::Secure(uint8_t _EESec) {
  if(!txPool[0]) return INONE;
  int i = 0;
  int y = 0;
  int Ret = 0xFFFF;
  uint8_t EESec = _EESec;
  // i = 0 to 15 (Change: 4/7/ Secure Code must be smaller than 0x7F)
  // Use top bit for Size [ Size is an int ]
  // bit: 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
  //      s0  z0  s1  z1  s2  z2  s3  z3  s4  z4  s5  z5  s6  z6  z7  z8
  while (i < 14) {
    bitWrite(Ret, i, bitRead(EESec, y));i++;
    bitWrite(Ret, i, bitRead(Size, y));i++;y++;
  }
  bitWrite(Ret, i, bitRead(Size, y));
  y++;
  i++;                                 // i=14, y=7 Sets bit 14 to Size-bit 7[8]
  bitWrite(Ret, i, bitRead(Size, y));  // i=15, y=8 Sets bit 15 to Size-bit 8[9]

  txPool[0][PKB_SECH] = highByte(Ret);  //3
  txPool[0][PKB_SECL] = lowByte(Ret);   //4
  return Ret;
}
//-----------------------------------------------------------------------------------------------------
/*
void DITSEngine::TxPacket::AddValue(uint8_t _DITidx, int _Value) {
  TxByte(_DITidx);
  TxByte(highByte(_Value));
  TxByte(lowByte(_Value));
}
*/

//-------------------------------------------------------------------------------------------------
void DITSEngine::RxProcessPacket() {
  if (!rxPacket) return;

  uint8_t targetNodeidx = BNONE;
  if (!rxPacket->IsREQ()) {targetNodeidx = FindNodeByRF(rxPacket->FromRF());}

  //------------------------- DEVITBL DATA ( No version control )----------------------------------------------------------
  if (rxPacket->Type() == PKT_DEVITBL) {                        // Received DEVITBL Data from targetnode
    DBRFINFO(("DITSEngine::RxProcessPacket PKT_DEVITBL"))
    
    // If no target node exist create one.
    if (targetNodeidx == BNONE) { 
      for (NodeDIT node(this); node.IsValid(); node.Next()) {if (node.IsDeleted()) targetNodeidx = node.DITidx();}
      if (targetNodeidx == BNONE) {targetNodeidx = AddNDIT();}
    }   
    NodeDIT targetnode(this, targetNodeidx);
    targetnode.NRFAddrH(rxPacket->FromRFH());
    targetnode.NRFAddrL(rxPacket->FromRFL());
    targetnode.NDITVer(rxPacket->NDITVer());
    char name[DITNameChar];                           // alloc USRBYTES
      for (int i=0; i<DITNameBytes; i++) {name[i] = rxPacket->rxPool[0][PKB_DEVITBL+i];} // write 10ch
      name[DITNameBytes] = '\0';                         // string terminate for strlen()
      targetnode.NSetName(name);                                 // write 'name'
    }
    // Todo: Add to save Device Information
    // Todo: Call for updates.
  
  //------------------------- DEVITBL REQUEST ( No version control )-------------------------------------------------------
  if (rxPacket->Type() == PKT_REQDEVITBL) {
    DBINFO(("DITSEngine::RxProcessPacket PKT_REQDEVITBL"))
    //TxPacket Pkt(EEPROM.read(EMC_SECNET), PKT_GOTCONFIG, rxPacket->FromRF(), TargetTode->Version());  // Create TxPacket
    //Pkt.AddTodeConfig(TargetTode->EEAddress());               // Load TxPacket with Configuration
    //RF->Send(&Pkt);                                           // Send Reply
    //delete (rxPacket);
    //rxPacket = 0;
    //return;  // Exit
  }
/*
  //-------------------------------- VERSION MATCH ------------------------------------------------------------------
  if (TargetTode->Version() != rxPacket->Version()) {  // Check Version MATCH
    DBRFAAERROR((TargetTode->Version()), (rxPacket->Version()),("Sys::RFLoop Tode PACKET Version Mismatch(TodeVer,PktVer)"))  // Show MISMATCH
    //TxPacket Pkt(EEPROM.read(EMC_SECNET), PKT_GOTCONFIG, rxPacket->FromRF(), TargetTode->Version());  // MISMATCH Tx Update Config
    //Pkt.AddTodeConfig(TargetTode->EEAddress());               // Tx Add Tode Config
    //RF->Send(&Pkt);                                           // Send Tode Config
    delete (rxPacket);
    rxPacket = 0;
    return;  // Exit
  } else {
    DBRFINFO(("Sys::RFLoop() TargetTode->Version() == rxPacket->Version()"))
  }
*/
  //-------------------------------- SINGLE DEVICE -------------------------------------------------------------------
  if (rxPacket->Type() == PKT_SETVAL || rxPacket->Type() == PKT_VAL) {
    /*
    Device* TargetDev = 0;
    int rfid = rxPacket->RFID();
    if (0 <= rfid && rfid < AEB_MAXDEVICES) TargetDev = TargetTode->Devices[rfid];  // Get TargetDev
    if (TargetDev == 0) {                                                           // Check TargetDev
      DBERROR(("Sys::RFLoop TargetDev==0"))
      delete (rxPacket);
      rxPacket = 0;
      return;  // ERROR Exit
    }

    if (rxPacket->Type() == PKT_SETVAL) {
      DBINFO(("Sys::RFLoop PKT_SETVAL"))
      TargetDev->Value(rxPacket->SetValue(), STSRFSET);  // Set Device Value & Reply
      TxPacket Pkt(EEPROM.read(EMC_SECNET), PKT_VAL, rxPacket->FromRF(), TargetTode->Version(), TargetDev->RFID, TargetDev->Value());  // GOTVAL the Set Value
      RF->Send(&Pkt);                                                            // Send the Reply

    } else if (rxPacket->Type() == PKT_VAL) {
      DBINFO(("Sys::RFLoop PKT_VAL"))
      TargetDev->Value(rxPacket->Value(TargetDev->RFID), STSRFGOT);  // Set GOT Value
    }
    */
    delete (rxPacket);
    rxPacket = 0;
    return;
  }

  //-------------------------------- MULTI DEVICE ---------------------------------------------------------------------
  if (rxPacket->Type() == PKT_REQVALS) {
    DBRFINFO(("Sys::RFLoop PKT_REQVALS\n"))
    //TxPacket Pkt(EEPROM.read(EMC_SECNET), PKT_VALS, rxPacket->FromRF(), TargetTode->Version());
    //for (int i = 0; i < AEB_MAXDEVICES; i++) {                // Append every Device Value
      //if (TargetTode->Devices[i] != 0) {                      // Iterate Devices[]
        //if (TargetTode->Devices[i]->RFID < AEB_MAXDEVICES) {  // Check Device RFID
          //Pkt.AddValue(TargetTode->Devices[i]->RFID, TargetTode->Devices[i]->Value());
        //}
      //}
    //}
    //RF->Send(&Pkt);  // Send Packet

  } else if (rxPacket->Type() == PKT_VALS) {
    DBRFINFO(("Sys::RFLoop PKT_VALS"))
    //for (int i = 0; i < AEB_MAXDEVICES; i++) {  // Iterate Devices
      //if (TargetTode->Devices[i] != 0) {        // Assign Device Value
        //TargetTode->Devices[i]->Value(rxPacket->Value(TargetTode->Devices[i]->RFID), STSRFGOT);
        //if (CurrList == TargetTode) TargetTode->Devices[i]->DisplayValue();  // Update Display
        //DBAINFO(("Sys::RFLoop PKT_VALS RFID: "), (TargetTode->Devices[i]->RFID))
      //}
    //}
  }
  // Delete Packet after Processing
  delete (rxPacket);
  rxPacket = 0;
}
//-----------------------------------------------------------------------------------------------------
//void DITSEngine::SaveNodeConfig() {
/*
  if (!rxPacket) return;  // semi-forced object check

  // --- Build NodeDIT from packet ---
  NodeDIT node;
  // Example mapping: RFH, RFL, Ver, Name[10]
  node.RFH = rxPacket->Bytes[PKB_FROM_RFH];
  node.RFL = rxPacket->Bytes[PKB_FROM_RFL];
  node.Ver = rxPacket->Version();
  char name[DIT_NAME_USRBYTES];name[DITTBL_NAME_BYTES]=0;
  for (int i = 0; i < DITTBL_NAME_BYTES; i++) {
    name[i] = (PKB_DEVITBL + i < rxPacket->Size) ? rxPacket->Bytes[PKB_DEVITBL + i] : 0;
  }
  node.NSetName(name);

  // Write Node to EEPROM via ManagerDIT
  int nodeResult = WriteNode(&node);  // idx=-1 -> find free automatically
  if (nodeResult == -1) {
    DBERROR(("SaveTodeConfig: Node write failed, no room\n"));
    return;
  }

  // --- Build DeviceDITs from packet ---
  int uint8_tIdx = PKB_DEVITBL + 10;  // start after Node Name
  while (uint8_tIdx + sizeof(DeviceDIT) <= Size) {
    DeviceDIT dev;
    dev.DNodeIdx = nodeResult;  // associate with this Node
    dev.DevType = rxPacket->Bytes[uint8_tIdx];
    dev.DevIdx = rxPacket->Bytes[uint8_tIdx + 1];
    for (int i = 0; i < 10; i++) {
      dev.Name[i] = rxPacket->Bytes[uint8_tIdx + 2 + i];
    }
    for (int i = 0; i < 6; i++) dev.Reserved[i] = 0xFF;

    int devResult = WriteDevice(&dev);
    if (devResult == -1) {
      DBERROR(("SaveTodeConfig: Device write failed, no room\n"));
      break;  // stop writing devices if pool full
    }

    uint8_tIdx += sizeof(DeviceDIT);
  }
  */
//}
//-----------------------------------------------------------------------------------------------------
void DITSEngine::TxSendThisNodeDEVITBL(uint16_t RFAddr, uint8_t nodeIdx, uint8_t _SecNet) {
/*
  TxPacket pkt(_SecNet, PKT_DEVITBL, RFAddr);
  int i = 0;

  NodeDIT node;
  if (!ReadNode(nodeIdx, &node)) {
    DBERRORAL(("TxSendThisNodeDEVITBL: Node not found idx="), (nodeIdx));
    return;
  }

  // --- Stream Node Version and Name ---
  pkt.TxByte(node.Ver);
  for (int i = 0; i < 10; i++) {
    pkt.TxByte(node.Name[i]);
  }

  // --- Stream Devices for this Node ---
  BeginDeviceIter(nodeIdx);
  DeviceDIT dev;
  while (NextDevice(&dev)) {
    pkt.TxByte(dev.DevType);
    pkt.TxByte(dev.DevIdx);
    for (int i = 0; i < 10; i++) pkt.TxByte(dev.Name[i]);
    for (int i = 0; i < 6; i++) pkt.TxByte(dev.Reserved[i]);
  }

  // Set FromRF
  unsigned int FromRF = Address();
  if (FromRF == 0xFFFF) {
    DBERROR(("Euint8_t::Send(Tx) FromRF=0xFFFF"))
    return;
  }
  pkt.Bytes[PKB_FROM_RFH] = highByte(FromRF);
  pkt.Bytes[PKB_FROM_RFL] = lowByte(FromRF);

  // Set Channel & Secure
  pkt.Bytes[PKB_CHANNEL] = Channel();
  pkt.Secure();

  // Re-Pack Bytes into Send Queue and show DEBUG INFO
  ArduinoQueue<uint8_t> bSendQueue(518);
  DBAAINFO(("To[0][1]: "), (pkt.Bytes[PKB_TO_RFH], HEX), (pkt.Bytes[PKB_TO_RFL], HEX))
  DBAINFO(("Channel[2]: "), (pkt.Bytes[PKB_CHANNEL], HEX))
  DBAAINFO(("SecNet,Size: "), (_SecNet), (pkt.Size))
  DBAAINFO(("SecNet,Size[3][4]: "), (pkt.Bytes[PKB_SECH], HEX), (pkt.Bytes[PKB_SECL], HEX))
  DBAINFO(("HSize+PacketType[5]: "), (pkt.Bytes[PKB_TYPE], HEX))
  DBAAINFO(("From[6][7]: "), (pkt.Bytes[PKB_FROM_RFH], HEX), (pkt.Bytes[PKB_FROM_RFL], HEX))
  DBAINFO(("Tode Version[8]: "), (pkt.Bytes[PKB_DITVER], HEX))
  //#define PKB_TYPE        5     // This point forward may differ
  //#define PKB_FROM_RFH    6
  //#define PKB_FROM_RFL    7
  //#define PKB_DITVER     8
  //#define PKB_DEVITBL  9     // Start Tode Config Data
  //#define PKB_DEVUID        9     // First RFID on GETVALS
  //#define PKB_VALUEH      10
  //#define PKB_VALUEL      11

  i = 0;
  uint8_t Temp = 0;
  while (i < pkt.Size) {
    if (i % 12 == 0) { DBENTER((" ")) }  // new line
    if (i < PKC_STATICBYTES) {
      bSendQueue.enqueue(pkt.Bytes[i]);
      DBENTERAA(("Tx"), (i), (pkt.Bytes[i], HEX))
    } else {
      if (pkt.ExtraBytes == 0) {
        DBERRORAL(("Euint8_t::Send pkt.ExtraBytes == 0 @i: "), (i))
        break;
      } else {
        if (pkt.ExtraBytes->isEmpty()) {
          DBINFO(("Euint8_t::Send pkt.ExtraBytes->isEmpty()"))
          break;
        }
        Temp = pkt.ExtraBytes->dequeue();
        DBENTERAA(("Ex"), (i), (Temp))
        bSendQueue.enqueue(Temp);
      }
    }
    i++;
  }
  DBENTER((""))

  // Send the Send-Queue
  i = 0;
  while (!bSendQueue.isEmpty()) {
    i++;
    if (i % PKC_STATICBYTES == 0) {
      //RFSERIAL.flush();
      //while ( digitalRead(PinAUX) == LOW ) { };     // Wait for not busy to send another 512-uint8_ts
      TxData(pkt.Bytes[PKB_TO_RFH]);  // Resend Header and continue
      TxData(pkt.Bytes[PKB_TO_RFL]);
      TxData(pkt.Bytes[PKB_CHANNEL]);
    }
    TxData(bSendQueue.dequeue());
  }
  */
}
//_____________________________________________________________________________________________________________________
#endif