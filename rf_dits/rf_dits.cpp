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
byte DITSEngine::SecNetCode() const { return pmem_read(pmSecNetAddr); }
//-------------------------------------------------------------------------------------------------
bool DITSEngine::SecNetCode(byte _SecNetCode) {
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
bool DITSEngine::AddThisNodeDevice(byte devType, const char* name) {
  if (!name) {DBDITERROR(("DITSEngine::AddThisNodeDevice !name.")) return false;}
  DBDITAAENTER((devType),(name),("DITSEngine::AddThisNodeDevice(<devNodeIdx>,devType,<name>)"))
  
  uint32_t devUIDMask[(DDITStopIdx + 31) / 32];   // Dynamically created a bitmask array for used devUIDs
  memset(devUIDMask, 0, sizeof(devUIDMask));      // Set all bytes in the array to 0
  
  uint16_t freeDITidx = BNONE;
  for (DeviceDIT dev(this); dev.IsValid(); dev.Next()) {
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
  DeviceDIT dev(this,freeDITidx); // Now, create the DeviceDIT struct and assign the values
  dev.DNodeIdx(0);                // Associate locally with "this node"(0)
  dev.DevUID(devUID);             // Assign the DevUID found above
  dev.DevType(devType);           // Assign devType
  dev.DSetName(name);             // Assign the name
  return true;                    // Return success
} 
//-------------------------------------------------------------------------------------------------
bool DITSEngine::DelThisNodeDevice(byte _devUID) {
  for(DeviceDIT dev(this); dev.IsValid(); dev.Next()) {
    if (dev.DNodeIdx()!=0) continue;
    if (dev.DevUID()==_devUID) {dev.IsDeleted(true);return true;}
  }
  DBAINFO((_devUID),("DITSEngine::DelThisNodeDevice(<devUID>)\n"))
  return false;  // Return false if the device not found.
}
//########################## 2. DITS Public RF Management.########################################
void DITSEngine::ProcessLoop() {
  
  // Process Rx Packet if needed.
  if (rxPacket && rxPacket->IsComplete(millis())) {
    if (rxPacket->IsValid()) RxProcessPacket();
    delete rxPacket;
    rxPacket = nullptr;  // Delete the Packet
  }

  // Send Tx Packet if needed.
  if (txPacket) {
    if (!txPacket->IsSecured()) txPacket->Secure();
    if (txPacket->IsSecured()) {
      for (int idx=0; idx < txPacket->rfPoolIdxs && txPacket->Size > 0; idx++) {
        if (!txPacket->rfPool[idx]) continue;
        int BytesInChunk = (txPacket->Size < txPacket->uiRadioPktMaxBytes) ? txPacket->Size : txPacket->uiRadioPktMaxBytes;
        for (int y=0; y < BytesInChunk; y++) {
          TxData(txPacket->rfPool[idx][y]);txPacket->Size--;}                           // Send one chunk.
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
  //RxPacket(byte _RadioPktMaxBytes, byte _SecNet, uint32_t _expireMillis = 30000);
  if (!rxPacket) rxPacket = new RxPacket(mRadioPktMaxBytes, SecNetCode());
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
  txPacket->pktREQDEVITBL();
  DBRFAINFO((RFAddr,HEX),("DITSEngine::TxAddRemoteNode(<RFAddr>) packet staged.\n"));
  return true;
}
//-------------------------------------------------------------------------------------------------
void DITSEngine::DelRemoteNode(byte nodeIdx) {
  NodeDIT node(this, nodeIdx); node.IsDeleted(true);
  for(DeviceDIT dev(this); dev.IsValid(); dev.Next()) {if (dev.DNodeIdx()==nodeIdx) dev.IsDeleted(true);}
}
//-------------------------------------------------------------------------------------------------
bool DITSEngine::TxSetRemoteDevVal(uint16_t nodeIdx, byte devUID, int value) {
  if (txPacket) {DBRFERROR(("DITSEngine::TxSetRemoteDevVal 'txPacket' Tx Busy.\n")) return false;}
  NodeDIT tonode(this,nodeIdx);
  NodeDIT thisnode(this,0);
  txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
  txPacket->ToFrom(tonode.NRFAddr(),thisnode.NRFAddr());
  txPacket->pktSETVAL(tonode.NDITVer(),devUID,value);
  DBRFAAAINFO((nodeIdx),(devUID),(value),("DITSEngine::TxSetRemoteDevVal(<nodeIdx>,<devUID>,<val>) PKT_SETVAL setup.\n"))
  return true;
}
//########################## 3. DITS Private Local Management.########################################
void DITSEngine::ScanPmemForDITSTOP() {
  bool StopFound = false;
  for (byte idx=0; idx<pmMaxDITRecords; ++idx) {
    if (pmem_read(pmNDITAddr(idx))==PMDITSTOP) {NDITStopIdx=idx; break;}
  }
  if (NDITStopIdx==0) {pmem_write(pmNDITAddr(0), PMDITSTOP);}
  for (byte idx=0; idx<pmMaxDITRecords; ++idx) {
    if (pmem_read(pmDDITAddr(idx))==PMDITSTOP) {DDITStopIdx=idx; break;}
  }
  if (DDITStopIdx==0) {pmem_write(pmDDITAddr(0), PMDITSTOP);}
}
//-----------------------------------------------------------------------------------------------------
byte DITSEngine::FindNodeDIT(uint16_t rfAddr, bool NotFoundAdd) {
  byte rfLow = rfAddr & 0xFF;          // extract low byte
  byte rfHigh = (rfAddr >> 8) & 0xFF;  // extract high byte
  byte delDIT = BNONE;
  for(NodeDIT node(this); node.IsValid(); node.Next()) {
    if(node.IsDeleted()) {if(delDIT==BNONE){delDIT=node.DITidx();} continue;}
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
byte DITSEngine::FindDeviceDIT(byte _DNodeIdx, byte _devUID, bool NotFoundAdd) {
  byte delDIT = BNONE;
  for(DeviceDIT dev(this); dev.IsValid(); dev.Next()) {
    if(dev.IsDeleted()) {if(delDIT==BNONE){delDIT=dev.DITidx();} continue;}
    if(dev.DNodeIdx()!=_DNodeIdx) continue;
    if(dev.DevUID()!=_devUID) continue;
    return dev.DITidx();
  }
  if (NotFoundAdd) {
    if (delDIT!=BNONE) {
      DeviceDIT dDIT(this,delDIT);
      dDIT.DNodeIdx(_DNodeIdx);
      return dDIT.DITidx();
    }
    return AddDDIT(_DNodeIdx);
  }
  return BNONE;
}
//-------------------------------------------------------------------------------------------------
byte DITSEngine::AddNDIT() {
  if ( DDITStopIdx+NDITStopIdx+2 > pmMaxDITRecords) {   // Check DIT Record boundary
    DBDITERROR(("DITSEngine::AddNDIT DDITStopIdx+NDITStopIdx+2>pmMaxDITRecords Out of Memory.\n"))
    return BNONE;
  }
  pmem_write(pmNDITAddr(NDITStopIdx) + PMO_NODEIDX, NDITStopIdx); // write NodeIdx at last STOP
  NDITStopIdx++;                                                  // Incr NDIT STOP
  pmem_write(pmNDITAddr(NDITStopIdx) + PMO_NODEIDX, PMDITSTOP);   // write direct, avoid IsValid trap.
  return NDITStopIdx - 1;
}
//-------------------------------------------------------------------------------------------------
byte DITSEngine::AddDDIT(byte _DNodeIdx) {
  if ( DDITStopIdx+NDITStopIdx+2 > pmMaxDITRecords) {   // Check DIT Record boundary
    DBDITERROR(("DITSEngine::AddDDIT DDITStopIdx+NDITStopIdx+2>pmMaxDITRecords Out of Memory.\n"))
    return BNONE;
  }
  pmem_write(pmDDITAddr(DDITStopIdx) + PMO_DNODEIDX, _DNodeIdx);  // write DNodeIdx at last STOP
  DDITStopIdx++;                                                  // Incr DDIT STOP
  pmem_write(pmDDITAddr(DDITStopIdx) + PMO_DNODEIDX, PMDITSTOP);  // write direct, avoid IsValid trap.
  return DDITStopIdx - 1;
}
//########################## 4. DITS Private RF Management.########################################
bool DITSEngine::RxProcessPacket() {
  if(!rxPacket) {DBRFERROR(("DITSEngine::RxProcessPacket !rxPacket.\n")) return false;}
  DBRFENTER(("DITSEngine::RxProcessPacket\n"))
  NodeDIT thisnode(this,THISNODE);
  
  // DIT version matching not required.
  if(rxPacket->PktType()==PKT_REQDEVITBL) {return TxSendThisNodeDEVITBL(rxPacket->FromRF());}
  if(rxPacket->PktType()==PKT_DEVITBL) {return RxSaveNodeDEVITBL();}

  // DIT Versions match check.
  NodeDIT fnode(this,FindNodeDIT(rxPacket->FromRF()));
  if(rxPacket->NDITVer()!=fnode.NDITVer()) {
    if(txPacket) {DBRFERROR(("DITSEngine::RxProcessPacket 'txPacket' Tx is busy.\n")) return false;}  
    txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
    txPacket->ToFrom(rxPacket->FromRF(), thisnode.NRFAddr());
    txPacket->pktREQDEVITBL();
    return false;
  }
  
  if (rxPacket->IsREQ()) {
    
    if(rxPacket->PktType()==PKT_SETVAL) {   // SETVAL only needs Tx if DevATTR is RWSS(SetSecured)
      bool RWSS = false; byte FromNode = FindNodeDIT(rxPacket->FromRF()); if (FromNode==BNONE) return false;
      for (DeviceDIT dev(this); dev.IsValid(); dev.Next()) {                         // Find devices DIT
        if (dev.IsDeleted()) continue; if (dev.DNodeIdx()!=FromNode) continue;
        if (dev.DevUID()==rxPacket->DevUID()) {RWSS = (dev.DevAttr() & 0xC0)==0xC0;} // Check if RWSS
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
    }

    // All below require Tx.
    if(txPacket) {DBRFERROR(("DITSEngine::RxProcessPacket REQ txPacket Tx is busy.\n")) return false;}
    txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
    txPacket->ToFrom(rxPacket->FromRF(), thisnode.NRFAddr());

    if(rxPacket->PktType()==PKT_REQVALS) {  
      txPacket->pktVALS(thisnode.NDITVer());
      DeviceDIT dev(this);
      for(DeviceDIT dev(this); dev.IsValid(); dev.Next()) {
        if (dev.IsDeleted()) continue; if (dev.DNodeIdx()!=THISNODE) continue;
        txPacket->AddTxByte(dev.DevUID());            // 1st - devUID
        int tmpval = RxReqDeviceValue(dev.DevUID());  // 2nd - value
        txPacket->AddTxByte(highByte(tmpval));txPacket->AddTxByte(lowByte(tmpval));  
      }
    }
    if(rxPacket->PktType()==PKT_REQVAL) {
      txPacket->pktVAL(thisnode.NDITVer(), rxPacket->DevUID(), RxReqDeviceValue(rxPacket->DevUID()));
    }
    if(rxPacket->PktType()==PKT_REQNONCE) { 
      int ChallengeResp = rxPacket->Value() ^ SecNetCode();
      txPacket->pktNONCERSP(thisnode.NDITVer(), rxPacket->DevUID(), ChallengeResp);
    }
    return true;
  }
  // DATA SUPPLY PACKETS--------------------------------------------------------
  if(rxPacket->PktType()==PKT_VALS) {
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
  }

  if(rxPacket->PktType()==PKT_VAL) {
    RxDataDevValue(FindNodeDIT(rxPacket->FromRF()), rxPacket->DevUID(), rxPacket->Value());
  }

  if(rxPacket->PktType()==PKT_NONCERSP) {
    // ---- Check Nonce before SETVAL activates ----
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
    SetValNonceDevUID = BNONE;
    return true;
  }
  // Todo: Call for updates.
}
//-----------------------------------------------------------------------------------------------------
bool DITSEngine::RxSaveNodeDEVITBL() {                        
  DBRFENTER(("DITSEngine::RxSaveNodeDEVITBL\n"))
   
  NodeDIT ditNode(this, FindNodeDIT(rxPacket->FromRF(),true));
  ditNode.NRFAddrH(rxPacket->FromRFH());
  ditNode.NRFAddrL(rxPacket->FromRFL());
  ditNode.NDITVer(rxPacket->NDITVer());

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
bool DITSEngine::TxSendThisNodeDEVITBL(uint16_t RFAddr) {
  if(txPacket) {DBRFERROR(("DITSEngine::TxSendThisNodeDEVITBL 'txPacket' is busy.\n")) return false;}
  DBRFAENTER((RFAddr,HEX),("DITSEngine::TxSendThisNodeDEVITBL(<RFAddr>)"))
  
  NodeDIT thisnode(this,THISNODE);
  txPacket = new TxPacket(mRadioPktMaxBytes,SecNetCode());
  txPacket->ToFrom(rxPacket->FromRF(), thisnode.NRFAddr());
  txPacket->pktDEVITBL(thisnode.NDITVer());
  char nname[DITNameChar];                                            //Node-Name first
  thisnode.NGetName(nname);                                           //RF DITVer in header.
  for (int i=0; i<DITNameChar; i++) {txPacket->AddTxByte(nname[i]);}
  for (DeviceDIT dev(this); dev.IsValid(); dev.Next()) {              //Devices Second.
    if(dev.IsDeleted()) continue;
    if(dev.DNodeIdx()==THISNODE) {
      txPacket->AddTxByte(dev.DevType());                             // DevType[0]
      txPacket->AddTxByte(dev.DevAttr());                             // DevAttr[1]
      txPacket->AddTxByte(dev.DevUID());                              // DevUID[2]
      char dname[DITNameChar];                                        // DevName[3...]
      dev.DGetName(dname);                                            // ...Repeat
      for (int i=0; i<DITNameChar; i++) {txPacket->AddTxByte(dname[i]);}
    }
  }
  return true;
}
//____________________________________________________________________________________________________________RxPacket
void DITSEngine::RxPacket::RxByte(byte _Byte) {
  uint16_t pi = NextIdx/uiRadioPktMaxBytes; uint16_t i = NextIdx-(pi*uiRadioPktMaxBytes);
  DBRFAAAENTER((pi),(i),(_Byte,HEX),("RxPacket::RxByte rfPool[<pi>][<i>]=<byte>\n"))
  if (!rfPool[pi]) {rfPool[pi] = new byte[uiRadioPktMaxBytes];}
  rfPool[pi][i] = _Byte;                                          // assign Byte in rfPool
  NextIdx++;                                                      // incr counter
  if (NextIdx == 5) {if (!IsSecure()) {PcktBegMS = 0; return;}}   // End on Insecure byte.
  if (NextIdx > 5 && NextIdx >= Size) {PcktBegMS = 0;}            // Flag Packet Done when Size is reached.
  return;
}
//-----------------------------------------------------------------------------------------------------
bool DITSEngine::RxPacket::IsSecure() {
  DBRFENTER(("RxPacket::IsSecure")) // Check - Good for 512b, 128-Combo SecNet, Add nonce bypass
  if (!rfPool[0]) return false;
  int rxSecNet = word(rfPool[0][PKB_SECH], rfPool[0][PKB_SECL]);  //0,1
  byte Sc = 0;uint16_t Sz = 0;int i = 0;int y = 0;

  while (i < 14) {
    bitWrite(Sc, y, bitRead(rxSecNet, i));i++;
    bitWrite(Sz, y, bitRead(rxSecNet, i));i++;
    y++;
  }
  bitWrite(Sz, y, bitRead(rxSecNet, i));y++;i++;
  bitWrite(Sz, y, bitRead(rxSecNet, i));
  DBRFAINFO((Sc, HEX),("RxPacket::IsSecure <Sc>\n"))
  DBRFAINFO((Sz),("RxPacket::IsSecure <Sz>\n"))
  if (Sc != SecNet) {DBRFERROR(("RxPacket::IsSecure FAILED SECURITY")) return false;}
  Size = Sz;
  return true;
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
  DBRFAENTER((SecNet,HEX),("TxPacket::Secure(<SecNet>)"))
  if(!rfPool[0]) {DBRFERROR(("TxPacket::Secure !rfPool[0]"))} return;
  byte pt = rfPool[0][PKB_TYPE];
  if ((PKT_REQ_MIN > pt || pt > PKT_REQ_MAX) || (PKT_DATA_MIN > pt || pt > PKT_DATA_MAX)) 
    {DBRFAERROR((pt,HEX),("TxPacket::Secure Unidentified Packet <Type>.\n")) 
    return;
  }
  int i = 0;
  int y = 0;
  int Ret = 0xFFFF;
  // i = 0 to 15 (Change: 4/7/ Secure Code must be smaller than 0x7F)
  // Use top bit for Size [ Size is an int ]
  // bit: 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
  //      s0  z0  s1  z1  s2  z2  s3  z3  s4  z4  s5  z5  s6  z6  z7  z8
  // Size is already adjusted in TxPacket Constructor
  while (i < 14) {
    bitWrite(Ret, i, bitRead(SecNet, y));i++;
    bitWrite(Ret, i, bitRead(Size, y));i++;y++;
  }
  bitWrite(Ret, i, bitRead(Size, y));
  y++;
  i++;                                 // i=14, y=7 Sets bit 14 to Size-bit 7[8]
  bitWrite(Ret, i, bitRead(Size, y));  // i=15, y=8 Sets bit 15 to Size-bit 8[9]

  rfPool[0][PKB_SECH] = highByte(Ret);  //3
  rfPool[0][PKB_SECL] = lowByte(Ret);   //4
  bSecured = true;
}
//_____________________________________________________________________________________________________________________
#endif