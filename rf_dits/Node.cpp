/******************************************************************************************************************//**
 * @file    Node.cpp
 * @brief   Ref.h
 *********************************************************************************************************************/
#ifndef _NODE_CPP
#define _NODE_CPP

#include "Node.h"
//#####################################################################################################################
Node::Node(byte _NodeIndex) {                       DBINITAL(("Node::Node"), (_NodeIndex))
  
  if ( _TodeIndex>9 ) { DBERRORAL(("Node::Node INVALID INDEX must be (0-9)!"),(_NodeIndex)) }
  else {
    NodeIndex = _NodeIndex;
    bIsLocal = (_TodeIndex == 0);
    //if ( bIsLocal ) Hardware = new HdwSelect(F("IO HDW"));           // Create a hardware select

    //MenuName(int _EENameAddress, bool _NameSettable = false);         
    NodeName = new MenuTodeName( EEAddress()+EMO_TODENAME, _TodeIndex);     // EEPROM Name Constructor
  }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
const char* Node::Title() { return TodeName->Name(); } 
bool Node::IsLocal() {                    return bIsLocal; }
unsigned int Node::RFAddr() {             unsigned int Ret=0; EEPROM.get(EEAddress(), Ret); return Ret; }
void Node::RFAddr(unsigned int _RFAddr) { EEPROM.put(EEAddress(),_RFAddr); }
byte Node::Version() {                    return EEPROM.read(EEAddress() + EMO_TODEVER); }
void Node::Version(byte _Version) {       if(_Version==255)_Version=0;EEPROM.update(EEAddress() + EMO_TODEVER,_Version); }
int Node::EEAddress() {                   return TodeIndex*AEB_TODEALLOC; }
//-----------------------------------------------------------------------------------------------------
void Node::EELoadDevices() {
    // LOAD DEVICES from EEPROM
    // Each Devices is (12)bytes
    // Byte Order is [ TYPE-BYTE . RFID-Byte . (10)Bytes NAME ]
    // With 30-Devices per Node = 360Bytes from EEPROM per Node + Devices

  // Clean Old Devices
  DelAllItems();
  for ( int i=0; i<AEB_MAXDEVICES; i++) { Devices[i]=0; }

  // Load Devices from EEPROM
  byte devType = BNONE;
  for ( int devIdx=0; devIdx<30; devIdx++ ) {
    int devEEStart = this->EEAddress()+AEB_TODEHEAD+(devIdx*AEB_DEVALLOC);
    devType = EEPROM.read(devEEStart);
    if ( devType==BNONE ) { 
      DBINFOA(("Node::EELoadDevices() - NO Device(devIdx)"),(devIdx))
      DBINFOAAL((" - EEPROM.read(EAddr)=(devType)"),(this->EEAddress()+AEB_TODEHEAD+(devIdx*AEB_DEVALLOC)),(devType,HEX))
    }
    else {
      byte devRFID = EEPROM.read(devEEStart+1);
      DBINFOAAL(("Node::EELoadDevices() *** Free Memory *** Pre-(devRFID) = "),(devRFID),(freeMem())) 
      if ( devRFID > 29 ) { DBERRORL(("Node::EELoadDevices() devRFID > 29")) continue; }
      this->AddDevice(devType, devRFID);
      DBINFOAAL(("Node::EELoadDevices() *** Free Memory *** Post-(devRFID) = "),(devRFID),(freeMem())) 
    }
  }  
}
//-----------------------------------------------------------------------------------------------------
void Node::Update() {                                                     DBENTERL(("Node::Update()"))

  if ( IsLocal() ) {
    DBINFOL(("Node::Update IS LOCAL"))
    // Loop local devices
    
    for ( int i=0; i<AEB_MAXDEVICES; i++ ) {
      if ( Devices[i]!=0 ) {
        Devices[i]->DisplayValue();
      }
    }
    
  
  } else {
    if ( RF==0 ) { DBERRORL(("Node::RFGetVals RF==0")) return; }
    RF->Send(new TxPacket( EEPROM.read(EMC_SECNET), PKT_GETVALS, RFAddr(), Version() ));
    //TxPacket(byte _SecNet, byte _Type, int _ToRF, byte _Ver=BNONE, byte _DevRFID=BNONE, int _Value = INONE) 
    for ( int i=0; i<AEB_MAXDEVICES; i++ ) {
      if ( Devices[i]!=0 ) { Devices[i]->Status(STSRFGETTING); }
    }
  }
}
//-----------------------------------------------------------------------------------------------------
void Node::DelDevice(MenuItem* _Item) {
DBENTERL(("Node::DelDevice(MenuItem* _Item)"))
  _Item->EEClear();                                 // Erases EEPROM for Device
  Del(_Item);                                       // Menu.cpp Deletes from Link-List
  for ( int i = 0; i<AEB_MAXDEVICES; i++ ) {
    if ( Devices[i] != 0 ) { 
      if ( Devices[i] == _Item ) { 
        DBINFOAL(("Node::DelDevice Devices[i] == _Item"),(i))
        Devices[i] = 0; 
      } 
    } 
  }
  Version(Version()+1);
}
//#####################################################################################################################
//-----------------------------------------------------------------------------------------------------
//_____________________________________________________________________________________________________________________
#endif