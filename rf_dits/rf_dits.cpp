/******************************************************************************************************************/ /**
 * @file    rf_dits.cpp
 * @brief   Ref.h
 *********************************************************************************************************************/
#ifndef _RF_DITS_CPP
#define _RF_DITS_CPP
#define CHALLENGE_WINDOW_MS 30000UL

#include "rf_dits.h"
// -------------------------------------------------------------------------------------------------
// TOC
// DITSPUBLOC 1. DITS Public Local Management.
// DITSPUBREM 2. DITS Public Remote Management.
// DITSPRVLOC 3. DITS Private Local Management.
// DITSPRVREM 4. DITS Private Remote Management.
// -------------------------------------------------------------------------------------------------
static byte     SetValNonceDevUID = 0;    // Staged Set Value DevUID
static int      SetValNonceValue = 0;     // Staged Set Value
static bool     s_rngSeeded = false;      // RF-DITS RNG Seed (static to this translation unit)
static uint16_t s_challengeNonce = 0;     // Static global to store nonce on each node (Target)
static uint32_t s_challengeIssuedMS = 0;  // Used to expire issued SetKey
static void RFDITS_SeedRNG() {
  if (s_rngSeeded) return; uint32_t s = micros(); uint32_t x;
  s ^= (uint32_t)&x; randomSeed(s);s_rngSeeded = true;
}
//########################## 1. DITS Public Local Management.########################################
DITSEngine::DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes)
    : pmSecNetAddr(_pmemBeginAddr), pmDITbase(_pmemBeginAddr+1), pmMaxDITRecords(_MaxDITRecords),
      pmDITend((_pmemBeginAddr+1)+(_MaxDITRecords*(PMO_NAME+_NameFieldBytes))), pmDITEndAddr(pmDITend+(PMO_NAME+_NameFieldBytes)),
      DITNameBytes(_NameFieldBytes), DITNameChar(_NameFieldBytes+1) { }
//-------------------------------------------------------------------------------------------------
void DITSEngine::begin() {
  for (byte idx=0; idx<pmMaxDITRecords && idx<256 ; ++idx) {if (pmem_read(pmNDITAddr(idx))==PMDITSTOP) {NDITStopIdx=idx; break;}}
  for (uint16_t idx=0; idx<pmMaxDITRecords; ++idx) {if (pmem_read(pmDDITAddr(idx))==PMDITSTOP) {DDITStopIdx=idx; break;}}
  if (NDITStopIdx==0) {
    pmem_write(pmNDITAddr(0), BNONE);           // To pass Valid test spot(0) will be deleted on fresh mem.
    pmem_write(pmNDITAddr(0) + PMO_NDITVER, 0); // Initial NDIT Version.
    pmem_write(pmNDITAddr(1), PMDITSTOP);       // Write STOP at (1).
    NDITStopIdx = 1;                            // First usable index is (0).
  }
  if (DDITStopIdx==0) {
    pmem_write(pmDDITAddr(0), BNONE);
    pmem_write(pmDDITAddr(1), PMDITSTOP);
    DDITStopIdx = 1;
  }
  DBDITINFO(("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n"))
  DBDITAINFO((DITNameBytes),("DITSEngine::begin <DITNameBytes>\n"))
  DBDITAINFO((DITNameChar),("DITSEngine::begin <DITNameChar>\n"))
  DBDITAINFO((pmSecNetAddr),("DITSEngine::begin <pmSecNetAddr>\n"))
  DBDITAINFO((pmMaxDITRecords),("DITSEngine::begin <pmMaxDITRecords>\n"))
  DBDITAINFO((NDITStopIdx),("DITSEngine::begin <NDITStopIdx>\n"))
  DBDITAINFO((DDITStopIdx),("DITSEngine::begin <DDITStopIdx>\n\n"))
  DBDITAINFO((pmDITbase),("DITSEngine::begin <pmDITbase>\n"))
  DBDITAINFO((pmDITend),("DITSEngine::begin <pmDITend>\n"))
  DBDITAINFO((pmDITEndAddr),("DITSEngine::begin <pmDITEndAddr>\n\n"))
  DBDITAINFO((pmDITEndAddr-pmSecNetAddr),("DITSEngine::begin Total Bytes Allocated for DIT Records.\n"))
  DBDITINFO(("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"))
}
//-------------------------------------------------------------------------------------------------
byte DITSEngine::SecNetCode() const { return pmem_read(pmSecNetAddr); }
//-------------------------------------------------------------------------------------------------
bool DITSEngine::SecNetCode(byte _SecNetCode) {
  if (_SecNetCode<=SECNETMAX) { pmem_write(pmSecNetAddr, _SecNetCode); return true; }
  return false;
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::UpdateThisNodeName(const char* name) {
  NodeDIT node(this,0);
  if (node.NDITidx == BNONE) {
    DBDITAERROR((node.NDITidx),("DITSEngine::UpdateThisNode <node(0).NDITidx>==BNONE"))
    return;
  }
  if (node.IsDeleted()) {node.IsDeleted(false);node.NodeIdx(0);}
  node.NSetName(name);
  node.NDITVer(node.NDITVer() + 1);
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::UpdateThisNodeRFAddr(uint16_t RFAddr) {
  NodeDIT node(this,0);
  if (node.NDITidx == BNONE) {
    DBDITAERROR((node.NDITidx),("DITSEngine::UpdateThisNode <node(0).NDITidx>==BNONE"))
    return;
  }
  if (node.IsDeleted()) {node.IsDeleted(false);node.NodeIdx(0);}
  node.NRFAddrH(highByte(RFAddr));
  node.NRFAddrL(lowByte(RFAddr));
  node.NDITVer(node.NDITVer() + 1);
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::AddThisNodeDevice(byte devType, byte devAttr, const char* name) {
  if (!name) {DBDITERROR(("DITSEngine::AddThisNodeDevice !name.")) return false;}
  DBDITAAENTER((devType),(name),("DITSEngine::AddThisNodeDevice(<devNodeIdx>,devType,<name>)\n"))
  
  uint32_t devUIDMask[(DDITStopIdx + 31) / 32];   // Dynamically created a bitmask array for used devUIDs
  memset(devUIDMask, 0, sizeof(devUIDMask));      // Set all bytes in the array to 0
  
  uint16_t freeDITidx = BNONE;
  for (DeviceDIT dev(this); dev.IsValid(); dev.NextAll()) {
    if(dev.IsDeleted()) {if (freeDITidx==BNONE) {freeDITidx = dev.DITidx();}  continue;}
    if(dev.DNodeIdx()==0) {
      byte thisUID = dev.DevUID();
      bitSet(devUIDMask[thisUID/32], thisUID%32);
    }
  }
  //------------------------------------------------------
  byte devUID = BNONE;
  for (byte UIDMaskIdx = 0; UIDMaskIdx < (DDITStopIdx + 31) / 32; ++UIDMaskIdx) {
    for (byte bitIdx = 0; bitIdx < 32; ++bitIdx) {     // Iterate through each bit
      if (!bitRead(devUIDMask[UIDMaskIdx], bitIdx)) {     // Check if the bit is not set
        devUID = UIDMaskIdx * 32 + bitIdx; break;         // break; Found the first available DevUID
      }
    }
    if (devUID != BNONE) { break; }
  }
  if (devUID == BNONE) {DBDITERROR(("DITSEngine::AddThisNodeDevice devUID==BNONE.\n")) return false;}
  //------------------------------------------------------
  if (freeDITidx == BNONE) { freeDITidx = AddDDIT(THISNODE); }
  NodeDIT node(this,THISNODE);
  DeviceDIT dev(this,freeDITidx); // Now, create the DeviceDIT struct and assign the values
  dev.DNodeIdx(0);                // Associate locally with "this node"(0)
  dev.DevUID(devUID);             // Assign the DevUID found above
  dev.DevAttr(devAttr);
  dev.DevType(devType);           // Assign devType
  dev.DSetName(name);             // Assign the name
  node.NDITVer(node.NDITVer() + 1);
  DBDITAAINFO((freeDITidx),(devUID),("DITSEngine::AddThisNodeDevice added-at <freeDITidx>,<devUID>\n"))
  return true;                    // Return success
} 
//-------------------------------------------------------------------------------------------------
bool DITSEngine::DelThisNodeDevice(byte _devUID) {
  DBDITAENTER((_devUID),("DITSEngine::DelThisNodeDevice(<devUID>)\n"))
  NodeDIT node(this,THISNODE);
  for(DeviceDIT dev(this); dev.IsValid(); dev.NextAll()) {
    if (dev.DNodeIdx()!=0) continue;
    if (dev.DevUID()==_devUID) {
      dev.IsDeleted(true);
      node.NDITVer(node.NDITVer() + 1);
      return true;
    }
  }
  DBDITAINFO((_devUID),("DITSEngine::DelThisNodeDevice(<devUID>) not found.\n"))
  return false;  // Return false if the device not found.
}
//-------------------------------------------------------------------------------------------------NodeDIT
bool DITSEngine::NodeDIT::Next() {
  for(byte i=NDITidx+1; i<pPtr->NDITStopIdx; i++) {
    if (pPtr->pmem_read(pPtr->pmNDITAddr(i))!=BNONE) {NDITidx = i;return true;}
  }
  return false;
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::NodeDIT::Prev() {
  byte i=NDITidx;
  while(i>0) {i--;if(pPtr->pmem_read(pPtr->pmNDITAddr(i))!=BNONE) {NDITidx = i;return true;}}
  return false;
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::NodeDIT::NGetName(char* buffer) const {
  if (!IsValid()) return;
  for (int i = 0; i < pPtr->DITNameBytes; i++) {buffer[i] = pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NAME + i);}
  buffer[pPtr->DITNameBytes] = '\0';
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::NodeDIT::NSetName(const char* value) {
  if (!IsValid()) return; byte len = strlen(value);
  for (int i = 0; i < pPtr->DITNameBytes; i++) {pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NAME + i, (i < len) ? value[i] : '\0');}
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::NodeDIT::IsDeleted() const {
  if (!IsValid()) return true; return pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX) == BNONE; 
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::NodeDIT::IsDeleted(bool deleteStatus) {
  if (!IsValid()) return true;
  if (deleteStatus && NDITidx != 0) {pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX, BNONE);}
  return pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX) == BNONE; 
}
//-------------------------------------------------------------------------------------------------DeviceDIT
bool DITSEngine::DeviceDIT::Next(byte nodeIdx) {
  for(uint16_t i=DDITidx+1; i<pPtr->DDITStopIdx; i++) {
    byte dnodeIdx = pPtr->pmem_read(pPtr->pmDDITAddr(i) + PMO_DNODEIDX);
    if (dnodeIdx==BNONE) continue;                      // Skip deleted
    if (nodeIdx!=BNONE && dnodeIdx!=nodeIdx) continue;  // Match on request.
    DDITidx = i;
    return true;
  }
  return false;
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::DeviceDIT::Prev(byte nodeIdx) {
  uint16_t i=DDITidx;
  if(i >= pPtr->DDITStopIdx) {i = pPtr->DDITStopIdx;}
  while(i>0) {
    i--;
    byte dnodeIdx = pPtr->pmem_read(pPtr->pmDDITAddr(i) + PMO_DNODEIDX);
    if (dnodeIdx==BNONE) continue;                      // Skip deleted
    if (nodeIdx!=BNONE && dnodeIdx!=nodeIdx) continue;  // Match on request.
    DDITidx = i;
    return true;
  }
  return false;
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::DeviceDIT::DGetName(char* buffer) const {
  if (!IsValid()) return;
  for (int i = 0; i < pPtr->DITNameBytes; i++) {buffer[i] = pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_NAME + i);}
  buffer[pPtr->DITNameBytes] = '\0';
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::DeviceDIT::DSetName(const char* value) {
  if (!IsValid()) return; byte len = strlen(value);
    for (int i = 0; i < pPtr->DITNameBytes; i++) {pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_NAME + i, (i < len) ? value[i] : '\0');}
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::DeviceDIT::IsDeleted() const {
  if (!IsValid()) return true; return pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX) == BNONE;
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::DeviceDIT::IsDeleted(bool deleteStatus) {
  if (!IsValid()) return true;
  if (deleteStatus) {pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX, BNONE);}
  return pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX) == BNONE;
}
//########################## 2. DITS Public RF Management.########################################
void DITSEngine::ProcessLoop() {
  
  // Process Rx Packet if needed.
  if (rxPacket && rxPacket->IsComplete(millis())) {
    if (rxPacket->IsValid()) {
      if (!RxProcessPacket()) {DBRFINFO(("DITSEngine::ProcessLoop !RxProcessPacket() false return.\n"))};
    }
    delete rxPacket;
    rxPacket = nullptr;  // Delete the Packet
  }

  // Send Tx Packet if needed.
  if (txPacket) {
    if (!txPacket->IsSecured()) txPacket->Secure();
    if (txPacket->IsSecured()) {
#if (DB_INFO && DB_RF)
      DBRFINFO(("DITSEngine::ProcessLoop txPacket->dumpRFPool...\n"))
      txPacket->dumpRFPool();
#endif
      for (int idx=0; idx < txPacket->rfPoolIdxs && txPacket->Size > 0; idx++) {
        if (!txPacket->rfPool[idx]) continue;
        int BytesInChunk = (txPacket->Size < txPacket->uiRadioPktMaxBytes) ? txPacket->Size : txPacket->uiRadioPktMaxBytes;
        for (int y=0; y < BytesInChunk; y++) {
          TxData(txPacket->rfPool[idx][y]);txPacket->Size--;                            // Send one chunk.
        }                           
        delete[] txPacket->rfPool[idx];txPacket->rfPool[idx] = nullptr;                 // Del chunk after sending.
        if (!TxReady(txPacket->ToRFAddr)) { break; }                                    // Check TxReady for new header per chunk.
      }                                                                                 // 'Size' allows return-to spot.
      if(txPacket->Size==0) {delete txPacket; txPacket = nullptr;}                      // Delete when empty.
    } else {
      DBRFERROR(("DITSEngine::ProcessLoop txPacket->Secure() failed.\n"))
      delete txPacket; txPacket = nullptr;
    }
  }
  // Next ProcessLoop stuff
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::RxData(byte _byte) {
  //ProcessLoop();            // Call ProcessLoop to ensure completes are processed.
  if (!rxPacket) rxPacket = new RxPacket(mRadioPktMaxBytes, SecNetCode(), RxExpireMillis);
  rxPacket->RxByte(_byte);
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::RadioPktMaxBytes(uint16_t maxBytes) {
  if (maxBytes >= PKC_RADIOPKTMAXBYTES_MIN && maxBytes <= PKC_RADIOPKTMAXBYTES_MAX) {
    mRadioPktMaxBytes = maxBytes; return true;}  // Success
  DBRFAERROR((maxBytes),("DITSEngine::RadioPktMaxBytes(<maxBytes>) is out of range.\n"))
  return false;  // Failure
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::TxAddRemoteNode(int RFAddr) {
  if (txPacket) {DBRFERROR(("DITSEngine::TxAddRemoteNode 'txPacket' still exists.\n")) return false;}
  NodeDIT node(this,0);
  txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
  txPacket->ToFrom(RFAddr, node.NRFAddr());
  txPacket->pktREQDITINFO();
  DBRFAINFO((RFAddr,HEX),("DITSEngine::TxAddRemoteNode(<RFAddr>) packet staged.\n"));
  return true;
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::DelRemoteNode(byte nodeIdx) {
  DBDITAENTER((nodeIdx),("DITSEngine::DelRemoteNode(<nodeIdx>)"))
  NodeDIT node(this, nodeIdx); node.IsDeleted(true);
  for(DeviceDIT dev(this); dev.IsValid(); dev.NextAll()) {if (dev.DNodeIdx()==nodeIdx) dev.IsDeleted(true);}
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::TxSetRemoteDevVal(byte nodeIdx, byte devUID, int value) {
  if (txPacket) {DBRFERROR(("DITSEngine::TxSetRemoteDevVal 'txPacket' Tx Busy.\n")) return false;}
  NodeDIT tonode(this,nodeIdx);
  NodeDIT thisnode(this,THISNODE);
  txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
  txPacket->ToFrom(tonode.NRFAddr(),thisnode.NRFAddr());
  txPacket->pktSETVAL(tonode.NDITVer(),devUID,value);
  DBRFAAAINFO((nodeIdx),(devUID),(value),("DITSEngine::TxSetRemoteDevVal(<nodeIdx>,<devUID>,<val>) PKT_SETVAL setup.\n"))
  return true;
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::TxGetRemoteDevVals(byte nodeIdx) {
  DBRFAENTER((nodeIdx),("DITSEngine::TxGetRemoteDevVals(<nodeIdx>)\n"))
  if (txPacket) {DBRFERROR(("DITSEngine::TxSetRemoteDevVal 'txPacket' Tx Busy.\n")) return false;}
  NodeDIT tonode(this,nodeIdx);
  NodeDIT thisnode(this,THISNODE);
  txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
  txPacket->ToFrom(tonode.NRFAddr(),thisnode.NRFAddr());
  txPacket->pktREQVALS(tonode.NDITVer());
}
//########################## 3. DITS Private Local Management.########################################
byte DITSEngine::FindNodeDIT(uint16_t rfAddr, bool NotFoundAdd) {
  DBDITAAENTER((rfAddr,HEX),(NotFoundAdd),("DITSEngine::FindNodeDIT(<rfAddr>,<NotFoundAdd>)\n"))
  byte rfLow = lowByte(rfAddr);    // extract low byte
  byte rfHigh = highByte(rfAddr);  // extract high byte
  byte delDIT = BNONE;
  for(NodeDIT node(this); node.IsValid(); node.NextAll()) {
    if(node.IsDeleted()) {
      DBDITAAINFO((node.NRFAddr()),(node.DITidx()),("DITSEngine::FindNodeDIT <RFAddr><DITidx> is deleted.\n"))
      if(delDIT==BNONE){delDIT=node.DITidx();} 
      continue;
    }
    if(node.NRFAddrL()!=rfLow) continue;
    if(node.NRFAddrH()!=rfHigh) continue;
    return node.DITidx();
  }
  if (NotFoundAdd) {
    if (delDIT!=BNONE) {NodeDIT dDIT(this,delDIT); dDIT.IsDeleted(false); return dDIT.DITidx();}
    return AddNDIT();
  } 
  return BNONE;
}
//-------------------------------------------------------------------------------------------------
uint16_t DITSEngine::FindDeviceDIT(byte _DNodeIdx, byte _devUID, bool NotFoundAdd) {
  DBDITAAAENTER((_DNodeIdx),(_devUID),(NotFoundAdd),("DITSEngine::FindDeviceDIT(<DNodeIdx>,<devUID>,<NotFoundAdd>)\n"))
  uint16_t delDIT = INONE;
  for(DeviceDIT dev(this); dev.IsValid(); dev.NextAll()) {
    if(dev.IsDeleted()) {if(delDIT==INONE){delDIT=dev.DITidx();} continue;}
    if(dev.DNodeIdx()!=_DNodeIdx) continue;
    if(dev.DevUID()!=_devUID) continue;
    DBDITINFO(("DITSEngine::FindDeviceDIT found DDIT.\n"))
    return dev.DITidx();
  }
  if (NotFoundAdd) {
    if (delDIT!=INONE) {
      DBDITINFO(("DITSEngine::FindDeviceDIT new DDIT.\n"))
      DeviceDIT dDIT(this,delDIT);
      dDIT.DNodeIdx(_DNodeIdx);
      return dDIT.DITidx();
    }
    DBDITINFO(("DITSEngine::FindDeviceDIT new AddDDIT().\n"))
    return AddDDIT(_DNodeIdx);
  }
  return INONE;
}
//-------------------------------------------------------------------------------------------------
byte DITSEngine::AddNDIT() {
  if ( DDITStopIdx+NDITStopIdx+2 > pmMaxDITRecords) {   // Check DIT Record boundary
    DBDITAAAERROR((DDITStopIdx),(NDITStopIdx),(pmMaxDITRecords),("DITSEngine::AddNDIT <DDITStopIdx>+<NDITStopIdx>+2><pmMaxDITRecords> Out of Memory.\n"))
    return BNONE;
  }
  pmem_write(pmNDITAddr(NDITStopIdx) + PMO_NODEIDX, NDITStopIdx); // write NodeIdx at last STOP
  NDITStopIdx++;                                                  // Incr NDIT STOP
  pmem_write(pmNDITAddr(NDITStopIdx) + PMO_NODEIDX, PMDITSTOP);   // write direct, avoid IsValid trap.
  return NDITStopIdx - 1;
}
//-------------------------------------------------------------------------------------------------
uint16_t DITSEngine::AddDDIT(byte _DNodeIdx) {
  if ( DDITStopIdx+NDITStopIdx+2 > pmMaxDITRecords) {   // Check DIT Record boundary
    DBDITERROR(("DITSEngine::AddDDIT DDITStopIdx+NDITStopIdx+2>pmMaxDITRecords Out of Memory.\n"))
    return INONE;
  }
  pmem_write(pmDDITAddr(DDITStopIdx) + PMO_DNODEIDX, _DNodeIdx);  // write DNodeIdx at last STOP
  DDITStopIdx++;                                                  // Incr DDIT STOP
  pmem_write(pmDDITAddr(DDITStopIdx) + PMO_DNODEIDX, PMDITSTOP);  // write direct, avoid IsValid trap.
  return DDITStopIdx - 1;
}
//########################## 4. DITS Private RF Management.########################################
///@brief Processes incoming DIT network packets.
///@details Handles version handshaking, security challenges (Nonces), and device value updates.
///@note If a packet requires a response, this function allocates a new TxPacket to the global @ref txPacket pointer.
///@return true if the packet was recognized and processed, false if an error occurred or the transmitter was busy.
bool DITSEngine::RxProcessPacket() {
  if(!rxPacket) {DBRFERROR(("DITSEngine::RxProcessPacket !rxPacket.\n")) return false;}
  DBRFAENTER((rxPacket->FromRF(),HEX),("DITSEngine::RxProcessPacket <fromRF>\n"))
#if (DB_INFO && DB_RF)
  DBRFINFO(("DITSEngine::RxProcessPacket rxPacket->dumpRFPool...\n"))
  rxPacket->dumpRFPool();
#endif
  NodeDIT thisnode(this,THISNODE);
  
  // DIT version matching not required.
  if(rxPacket->PktType()==PKT_REQDITINFO) {return TxSendThisNodeDITINFO(rxPacket->FromRF());}
  if(rxPacket->PktType()==PKT_DITINFO) {return RxSaveNodeDITINFO();}
  
  if(rxPacket->PktType()==PKT_REQNONCE) { 
    DBRFINFO(("DITSEngine::RxProcessPacket PKT_REQNONCE\n"))
    if(txPacket) {DBRFERROR(("DITSEngine::RxProcessPacket PKT_REQNONCE 'txPacket' Tx is busy.\n")) return false;}
    int ChallengeResp = rxPacket->Value() ^ SecNetCode();
    txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
    txPacket->ToFrom(rxPacket->FromRF(), thisnode.NRFAddr());
    txPacket->pktNONCERSP(thisnode.NDITVer(), rxPacket->DevUID(), ChallengeResp);
    return true;
  }

  // ---- Check Nonce before SETVAL activates ----
  if(rxPacket->PktType()==PKT_NONCERSP) {
    DBRFINFO(("DITSEngine::RxProcessPacket PKT_NONCERSP\n"))
    unsigned long nowMS = millis();
    if (rxPacket->Value() != s_challengeNonce) { 
      DBRFERROR(("DITSEngine::RxProcessPacket SETVAL nonce mismatch\n"))
      return false;
    }
    if (nowMS - s_challengeIssuedMS > CHALLENGE_WINDOW_MS) { 
      DBRFERROR(("DITSEngine::RxProcessPacket SETVAL nonce expired\n"))
      s_challengeNonce = 0; return false;
    }
    RxReqDeviceSet(SetValNonceDevUID, SetValNonceValue); 
    s_challengeNonce = 0;
    SetValNonceDevUID = BNONE;
    return true;
  }

  // *** DIT Versions match check. ***
  if(rxPacket->NDITVer()!=thisnode.NDITVer()) {
    DBRFINFO(("DITSEngine::RxProcessPacket rxPacket->NDITVer()!=thisnode.NDITVer() version mis-match.\n"))
    if(txPacket) {DBRFERROR(("DITSEngine::RxProcessPacket 'txPacket' Tx is busy.\n")) return false;} 
    return TxSendThisNodeDITINFO(rxPacket->FromRF());
  }
    
  if (rxPacket->PktType()==PKT_SETVAL) {
    DBRFINFO(("DITSEngine::RxProcessPacket PKT_SETVAL\n"))
    bool RWSS = false;
    for (DeviceDIT dev(this); dev.IsValid(); dev.NextAll()) {                 // Find devices DIT
      if (dev.IsDeleted()) continue; if (dev.DNodeIdx()!=THISNODE) continue;
      if (dev.DevUID()==rxPacket->DevUID()) {
        RWSS = (dev.DevAttr() & 0xC0) == 0x80; break;
      }  // Check if RWSS
    }
    if (!RWSS) {RxReqDeviceSet(rxPacket->DevUID(), rxPacket->Value()); return true;}
    if(txPacket) {DBRFERROR(("DITSEngine::RxProcessPacket REQ txPacket Tx is busy.\n")) return false;}
    txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
    txPacket->ToFrom(rxPacket->FromRF(), thisnode.NRFAddr());     // Setup TxPacket.
    RFDITS_SeedRNG();                                             // ensure random seeded (Generate Nonce)
    s_challengeNonce = (random(0, 0xFFFF) ^ (millis() & 0xFFFF)); // 16-bit nonce
    s_challengeIssuedMS = millis();                               // timestamp
    txPacket->pktREQNONCE(thisnode.NDITVer(), rxPacket->DevUID(), s_challengeNonce);  // Send Challenge
    SetValNonceDevUID = rxPacket->DevUID();                       // Stage Set Value Request
    SetValNonceValue = rxPacket->Value(); 
    s_challengeNonce ^= SecNetCode();                             // Stage expected response.
    return true;
  }
     
  if (rxPacket->PktType()==PKT_REQVALS) {
    DBRFINFO(("DITSEngine::RxProcessPacket PKT_REQVALS\n"))
    if(txPacket) {DBRFERROR(("DITSEngine::RxProcessPacket REQ txPacket Tx is busy.\n")) return false;}
    txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
    txPacket->ToFrom(rxPacket->FromRF(), thisnode.NRFAddr());
    txPacket->pktVALS(thisnode.NDITVer());
    DeviceDIT dev(this);
    for(DeviceDIT dev(this); dev.IsValid(); dev.NextAll()) {
      if (dev.IsDeleted()) continue; if (dev.DNodeIdx()!=THISNODE) continue;
      txPacket->AddTxByte(dev.DevUID());            // 1st - devUID
      int tmpval = RxReqDeviceValue(dev.DevUID());  // 2nd - value
      txPacket->AddTxByte(highByte(tmpval));txPacket->AddTxByte(lowByte(tmpval));  
    }
    return true;
  }

  if (rxPacket->PktType()==PKT_REQVAL) {
    DBRFINFO(("DITSEngine::RxProcessPacket PKT_PKT_REQVAL\n"))
    if(txPacket) {DBRFERROR(("DITSEngine::RxProcessPacket REQ txPacket Tx is busy.\n")) return false;}
    txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
    txPacket->ToFrom(rxPacket->FromRF(), thisnode.NRFAddr());
    txPacket->pktVAL(thisnode.NDITVer(), rxPacket->DevUID(), RxReqDeviceValue(rxPacket->DevUID()));
    return true;
  }

  if (rxPacket->PktType()==PKT_VALS) {
    DBRFINFO(("DITSEngine::RxProcessPacket PKT_VALS\n"))
    NodeDIT fnode(this,FindNodeDIT(rxPacket->FromRF()));
    byte devUID  = 0; byte valH = 0; uint16_t linear = 0; byte bytID = 0;
    for (int chunk = 0; chunk < rxPacket->rfPoolIdxs && linear < rxPacket->Size; chunk++) {
      if (!rxPacket->rfPool[chunk]) continue;
      for (int off = 0; off < rxPacket->uiRadioPktMaxBytes && linear < rxPacket->Size; off++, linear++) {
        if (linear < PKB_XDATA_BEG) continue;
        byte byt = rxPacket->rfPool[chunk][off];
        if (bytID==0) {devUID = byt;}
        else if (bytID==1) {valH = byt;}
        else if (bytID==2) {RxDataDevValue(fnode.NodeIdx(), devUID, word(valH,byt));}
        bytID = (bytID + 1) % 3;
      }
    }
    return true;
  }

  if (rxPacket->PktType()==PKT_VAL) {
    DBRFINFO(("DITSEngine::RxProcessPacket PKT_VAL\n"))
    RxDataDevValue(FindNodeDIT(rxPacket->FromRF()), rxPacket->DevUID(), rxPacket->Value());
    return true;
  }

  DBRFAERROR((rxPacket->PktType(),HEX),("DITSEngine::RxProcessPacket <rxPacket->PktType> not defined.\n"))
  // Todo: Call for updates.
}
//-----------------------------------------------------------------------------------------------------
bool DITSEngine::RxSaveNodeDITINFO() {                        
  DBRFENTER(("DITSEngine::RxSaveNodeDITINFO\n"))
   
  NodeDIT ditNode(this, FindNodeDIT(rxPacket->FromRF(),true));
  ditNode.NRFAddrH(rxPacket->FromRFH());
  ditNode.NRFAddrL(rxPacket->FromRFL());
  ditNode.NDITVer(rxPacket->NDITVer());

  // Mark previous dev records deleted.
  for (DeviceDIT dev(this); dev.IsValid(); dev.NextAll()) {
    if (dev.DNodeIdx()==ditNode.NodeIdx()) {dev.IsDeleted(true);}
  }

  byte  nameIdx   = 0;
  char nodeName[DITNameChar];
  char devName[DITNameChar];
  byte devType = 0; byte devAttr = 0; byte devUID  = 0;
  uint16_t linear = 0; byte  bytID = 0;
  
  for (int chunk = 0; chunk < rxPacket->rfPoolIdxs && linear < rxPacket->Size; chunk++) {
    if (!rxPacket->rfPool[chunk]) continue;
    for (int off = 0; off < rxPacket->uiRadioPktMaxBytes && linear < rxPacket->Size; off++, linear++) {
      if (linear < PKB_XDATA_BEG) continue;
      byte byt = rxPacket->rfPool[chunk][off];
      switch (bytID) {
        case 0: // ----- NODE NAME -----
          if (nameIdx < DITNameBytes) {nodeName[nameIdx++] = byt;}
          if (byt == '\0') {nodeName[DITNameBytes] = '\0'; ditNode.NSetName(nodeName); bytID = 1;nameIdx = 0;}
          break;
        case 1: // ----- DEV TYPE -----
          devType = byt;
          if (devType == 0) return true;   // end of table
          bytID = 2;
          break;
        case 2: // ----- DEV ATTR -----
          devAttr = byt;
          bytID = 3;
          break;
        case 3: // ----- DEV UID -----
          devUID = byt;
          bytID = 4;
          nameIdx = 0;
          break;
        case 4: // ----- DEV NAME -----
          if (nameIdx < DITNameBytes) devName[nameIdx++] = byt;
          if (byt == '\0') {
              devName[DITNameBytes] = '\0';
              DeviceDIT ditDev(this,FindDeviceDIT(ditNode.NodeIdx(),devUID,true));
              ditDev.DNodeIdx(ditNode.NodeIdx());
              ditDev.DevType(devType);
              ditDev.DevAttr(devAttr);
              ditDev.DevUID(devUID);
              ditDev.DSetName(devName);
              bytID = 1;  // expect next DevType
          }
          break;
      } // end switch.
    }
  }
}
//-----------------------------------------------------------------------------------------------------
bool DITSEngine::TxSendThisNodeDITINFO(uint16_t RFAddr) {
  if(txPacket) {DBRFERROR(("DITSEngine::TxSendThisNodeDITINFO 'txPacket' is busy.\n")) return false;}
  DBRFAENTER((RFAddr,HEX),("DITSEngine::TxSendThisNodeDITINFO(<RFAddr>)\n"))
  
  NodeDIT thisnode(this,THISNODE);
  txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
  txPacket->ToFrom(rxPacket->FromRF(), thisnode.NRFAddr());
  txPacket->pktDITINFO(thisnode.NDITVer());
  char nname[DITNameChar];                                            //Node-Name first
  thisnode.NGetName(nname);                                           //RF DITVer in header.
  for (int i=0; i<DITNameChar; i++) {
    if(nname[i]=='\0') break;
    txPacket->AddTxByte(nname[i]);
  }
  txPacket->AddTxByte('\0');
  for (DeviceDIT dev(this); dev.IsValid(); dev.NextAll()) {              //Devices Second.
    if(dev.IsDeleted()) continue;
    if(dev.DNodeIdx()==THISNODE) {
      txPacket->AddTxByte(dev.DevType());                             // DevType[0]
      txPacket->AddTxByte(dev.DevAttr());                             // DevAttr[1]
      txPacket->AddTxByte(dev.DevUID());                              // DevUID[2]
      char dname[DITNameChar];                                        // DevName[3...]
      dev.DGetName(dname);                                            // ...Repeat
      for (int i=0; i<DITNameChar; i++) {
        if(dname[i]=='\0') break;
        txPacket->AddTxByte(dname[i]);
      }
      txPacket->AddTxByte('\0');
    }
  }
  return true;
}
//____________________________________________________________________________________________________________RxPacket
void DITSEngine::RxPacket::RxByte(byte _Byte) {
  uint16_t pi = NextIdx/uiRadioPktMaxBytes; uint16_t i = NextIdx-(pi*uiRadioPktMaxBytes);
  if (!rfPool[pi]) {rfPool[pi] = new byte[uiRadioPktMaxBytes];}
  rfPool[pi][i] = _Byte;                                          // assign Byte in rfPool
  NextIdx++;                                                      // incr counter
  if (NextIdx == 3) {                                             // Packet Start Check.
    if (rfPool[0][0] < PKT_TYPEMIN) {PcktBegMS = 0;return;}       // Check valid PKT_TYPE byte[0]
    if (rfPool[0][0] > PKT_TYPEMAX) {PcktBegMS = 0;return;}
    IsSecure(); if(!bIsSecure) {PcktBegMS = 0;return;}            // Check byte[1]&[2] are valid.
  }   
  if (NextIdx > 3 && NextIdx >= Size) {PcktBegMS = 0;}            // Flag Packet Done when Size is reached.
}
//-----------------------------------------------------------------------------------------------------
void DITSEngine::RxPacket::IsSecure() {
  DBRFENTER(("RxPacket::IsSecure\n")) // Check - Good for 512b, 128-Combo SecNet, Add nonce bypass
  if (bIsSecure) return;
  if (!rfPool[0]) return;
  uint16_t rxSecNet = word(rfPool[0][PKB_SECH], rfPool[0][PKB_SECL]);  //0,1
  byte Sc = 0;uint16_t Sz = 0;byte i = 0;byte y = 0;

  while (i < 14) {                    
    bitWrite(Sc, y, bitRead(rxSecNet, i));i++; // at write 0,2,4,6,8,10,12
    bitWrite(Sz, y, bitRead(rxSecNet, i));i++; // at write 1,3,5,7,9,11,13 ++ to 14. !(14<14 exit)
    y++;                                       // at write 0,1,2,3,4,5 ,6  ++ to 7
  }
  bitWrite(Sz, y, bitRead(rxSecNet, i));i++;y++;  // y=7, i=14. ++ to y=8, ++ to i=15.
  bitWrite(Sz, y, bitRead(rxSecNet, i));          // writes i=15 into y=8(Size_uint16_t)
  DBRFAINFO((Sc, HEX),("RxPacket::IsSecure <Sc>\n"))
  DBRFAINFO((Sz),("RxPacket::IsSecure <Sz>\n"))
  if (Sc != SecNet) {DBRFERROR(("RxPacket::IsSecure FAILED SECURITY\n")) return;}
  Size = Sz;
  bIsSecure = true;
}
//____________________________________________________________________________________________________________TxPacket
void DITSEngine::TxPacket::AddTxByte(byte _TxByte) {
  if (!rfPool) {DBRFERROR(("TxPacket::AddTxByte !rfPool\n")) return;}
  if (Size>512) {DBRFERROR(("TxPacket::AddTxByte Size>512\n")) return;}
  byte currIdx = Size / uiRadioPktMaxBytes;
  byte currByte = Size % uiRadioPktMaxBytes;
  if (!rfPool[currIdx]) {rfPool[currIdx] = new byte[uiRadioPktMaxBytes];} // Initialize new radio chunks as needed.
  rfPool[currIdx][currByte] = _TxByte; Size++;
}
//-----------------------------------------------------------------------------------------------------
void DITSEngine::TxPacket::Secure() {
  DBRFAENTER((SecNet,HEX),("TxPacket::Secure(<SecNet>)\n"))
  if(bSecured) {DBRFERROR(("TxPacket::Secure Already Secured!\n")) return;}
  if(!rfPool[0]) {DBRFERROR(("TxPacket::Secure !rfPool[0]\n")) return;}
  if(rfPool[0][PKB_TYPE] < PKT_TYPEMIN || rfPool[0][PKB_TYPE] > PKT_TYPEMAX) {
    DBRFAERROR((rfPool[0][PKB_TYPE],HEX),("TxPacket::Secure Unidentified Packet <Type>.\n")) 
    return;
  }
  byte i = 0;
  byte y = 0;
  uint16_t Ret = 0xFFFF;
  // i = 0 to 15 (Change: 4/7/ Secure Code must be smaller than 0x7F)
  // Use top bit for Size [ Size is an int ]
  // bit: 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
  //      s0  z0  s1  z1  s2  z2  s3  z3  s4  z4  s5  z5  s6  z6  z7  z8
  // Size is already adjusted in TxPacket Constructor
  while (i < 14) {                            
    bitWrite(Ret, i, bitRead(SecNet, y));i++; // at write 0,2,4,6,8,10,12
    bitWrite(Ret, i, bitRead(Size, y));i++;   // at write 1,3,5,7,9,11,13 ++ to 14. !(14<14 exit)
    y++;                                      // at write 0,1,2,3,4,5 ,6  ++ to 7
  }
  bitWrite(Ret, i, bitRead(Size, y));i++;y++;   // y=7, i=14. Read Size[7] to Ret[14]
  bitWrite(Ret, i, bitRead(Size, y));           // y=8, i=15. Read Size[8] to Ret[15]

  rfPool[0][PKB_SECH] = highByte(Ret);  //3
  rfPool[0][PKB_SECL] = lowByte(Ret);   //4
  bSecured = true;
}
//_____________________________________________________________________________________________________________________
#endif