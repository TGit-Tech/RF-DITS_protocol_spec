/******************************************************************************************************************//**
 * @file    rf_dits.cpp
 * @brief   Ref.h
 *********************************************************************************************************************/
#ifndef _RF_DITS_CPP
#define _RF_DITS_CPP

#include "rf_dits.h"
//#####################################################################################################
//#####################################################################################################
RxPacket::RxPacket() {  }
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
byte    RxPacket::Type()      { return Bytes[PKB_TYPE]; }
int     RxPacket::FromRF()    { return word(Bytes[PKB_FROM_RFH],Bytes[PKB_FROM_RFL]); }
byte    RxPacket::Version()   { return Bytes[PKB_TODEVER]; }
byte    RxPacket::DITidx()    { return Bytes[PKB_DITidx]; }
int     RxPacket::SetValue()  { return word(Bytes[PKB_VALUEH],Bytes[PKB_VALUEL]); }
//-----------------------------------------------------------------------------------------------------
bool RxPacket::RxByte(byte _Byte) {
  DBINFOAAL(("RxPacket::RxByte(byte,NextIdx): "),(_Byte,HEX),(NextIdx))
  if ( NextIdx<58 ) { Bytes[NextIdx] = _Byte; }
  else { 
    if ( ExtraBytes == 0 ) ExtraBytes = new ArduinoQueue<byte>(512);
    if ( ExtraBytes->isFull() ) { DBERRORL(("RxPacket::RxByte ExtraBytes->isFull()")) return true; }
    ExtraBytes->enqueue(_Byte);
  }
  NextIdx++;
  if (NextIdx==5) { if (!IsSecure()) return true; }         //End on Insecure
  if (NextIdx>5 && NextIdx>=Size) return true;            //End
  return false;
}
//-----------------------------------------------------------------------------------------------------
bool RxPacket::IsSecure() {                                       DBENTERL(("RxPacket::IsSecure"))
  int SecNet = word(Bytes[PKB_SECH],Bytes[PKB_SECL]);   //0,1
  byte Sc = 0; int Sz = 0;int i = 0; int y = 0;

  while ( i<14 ) {bitWrite(Sc,y,bitRead(SecNet,i));i++;bitWrite(Sz,y,bitRead(SecNet,i));i++;y++;}
  bitWrite(Sc,y,bitRead(Size,i));y++;i++;
  bitWrite(Sz,y,bitRead(Size,i));
  
  DBINFOAL(("RxPacket::IsSecure Sc="),(Sc,HEX))
  DBINFOAL(("RxPacket::IsSecure Sz="),(Sz))
  if ( Sc != EEPROM.read(EMC_SECNET) ) { DBERRORL(("RxPacket::IsSecure FAILED SECURITY")) return false; }
  Size = Sz;
  return true;
}
//-----------------------------------------------------------------------------------------------------
bool RxPacket::IsValid() {
  if ( Size==0 || Type() == PKT_NOTSET) return false;
  return true;
}
//-----------------------------------------------------------------------------------------------------
bool RxPacket::IsREQ() {
  return bitRead(Type(),PKT_ISREQBIT);
}
//-----------------------------------------------------------------------------------------------------
int RxPacket::Value(byte _DITidx) {             DBENTERAL(("RxPacket::Value(GET) DITidx: "),(_DITidx))
  // Check bytes for DITidx then return Value
  for ( int i=PKB_DITidx; i<Size; i=i+3 ) {
    if ( Bytes[i]==_DITidx ) {
      DBINFOL(("RxPacket::Value(GET) DITidx Found."))
      return word(Bytes[i+1],Bytes[i+2]);
    }
  }
  DBERRORL(("RxPacket::Value(GET) DITidx NOT Found."))
  return VALNOTSET;
}
//-----------------------------------------------------------------------------------------------------
void RxPacket::SaveTodeConfig(int _EEAddress) {      
  DBENTERAL(("RxPacket::SaveTodeConfig(?_EEAddress?): "),(_EEAddress))
  //  TODEVER = 2
  EEPROM.update(_EEAddress+EMO_TODEVER, Version() );  
  DBINFOAAL(("RxPacket::SaveTodeConfig - Save Version EEPROM.update(?_EEAddress+EMO_TODEVER?, ?Version()? )"),(_EEAddress+EMO_TODEVER),(Version()))

  //#define PKB_TODEVER     8
  //#define PKB_TODECONFIG  9     // Start Node Config Dat
  DBINFOAL(("Byte Size: "),(Size))
  int iByte = PKB_TODECONFIG+1;               // Start Byte[] at TodeConfig +Version [10]
  byte DevConfig[AEB_DEVALLOC];for (int i=0;i<AEB_DEVALLOC;i++) {DevConfig[i]=0xFF;} 
  bool bDevConfigLoaded = false;
  byte DevConfigIndex = 0;
    
  // Write the TodeName
  for ( int EEAddr = _EEAddress+EMO_TODENAME; EEAddr < _EEAddress+EMO_TODENAME+AEB_TODENAME; EEAddr++ ) {
    if ( iByte > Size ) { DBERRORL(("RxPacket::SaveTodeConfig() Packet Size no TodeName")) return; }
    EEPROM.update(EEAddr,Bytes[iByte]);
    DBINFOAAL(("RxPacket::SaveTodeConfig() EEPROM.update(?EEAddr?,?Bytes[iByte]?) TODENAME "),(EEAddr),(Bytes[iByte],HEX))
    iByte++;
  }
  
  // Write Devices
  for ( int iEEDevPosIdx = 0; iEEDevPosIdx<AEB_MAXDEVICES; iEEDevPosIdx++ ) {   // Iterate all EE Device Position Indexes
    int EEAddr = _EEAddress+AEB_TODEHEAD+(AEB_DEVALLOC*iEEDevPosIdx);           // Starting EEAddr of Device

    // vvv[ Load the Next Device Config from Packet ]vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if ( !bDevConfigLoaded ) {
      for (int i=0;i<AEB_DEVALLOC;i++) {
        if ( iByte > Size ) { DevConfig[i] = 0xFF; }
        else {
          if ( iByte < 58 ) { 
            DevConfig[i]=Bytes[iByte]; 
            DBINFOAAL(("RxPacket::SaveTodeConfig() DevConfig[?]=?Bytes[iByte]?"),(i),(DevConfig[i],HEX))
          } else {
            if ( ExtraBytes == 0 ) { DBERRORL(("RxPacket::SaveTodeConfig ExtraBytes == 0")) return; }
            if ( ExtraBytes->isEmpty() ) { DBERRORL(("RxPacket::SaveTodeConfig ExtraBytes->isEmpty()")) return; }
            DevConfig[i] = ExtraBytes->dequeue(); 
            DBINFOAAL(("RxPacket::SaveTodeConfig() DevConfig[?]=?ExtraBytes->dequeue()?"),(i),(DevConfig[i],HEX))
          }
          iByte++;
        }
      }
      bDevConfigLoaded = true;      
    }
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    
    // Write Device Config either BLANK or in the proper EEPROM position.
    for (int i=0;i<AEB_DEVALLOC;i++) { 
      if ( DevConfig[1] == iEEDevPosIdx ) {
        EEPROM.update(EEAddr,DevConfig[i]); bDevConfigLoaded = false;
        DBINFOAAL(("RxPacket::SaveTodeConfig EEPROM.update(?EEAddr?,?DevConfig[i])?"),(EEAddr,HEX),(DevConfig[i],HEX))        
      } else {
        EEPROM.update(EEAddr,0xFF);
        DBINFOAAL(("RxPacket::SaveTodeConfig() EEPROM.update(?EEAddr?,0xFF) BLANK-DEV @?EEDevPos?"),(EEAddr,HEX),(iEEDevPosIdx))        
      }
      EEAddr++;
    }
  } // Next EEDevPosIdx
}
//#####################################################################################################
TxPacket::TxPacket(byte _SecNet, byte _Type, int _ToRF, byte _Ver, byte _DevDITidx, int _Value) {
  DBINITAAL(("TxPacket::TxPacket[.,ToRF,Ver,.,.]: "),((unsigned int)_ToRF,HEX),(_Ver))

  // Packet-Type to Byte-Size ( Size is Byte[i]+1 )
  if      ( _Type == PKT_REQCONFIG )  { DBINFOL(("TxPacket::TxPacket PKT_REQCONFIG")) Size = 8; }
  else if ( _Type == PKT_REQVALS )    { DBINFOL(("TxPacket::TxPacket PKT_REQVALS")) Size = 9; }
  else if ( _Type == PKT_REQVAL )     { DBINFOL(("TxPacket::TxPacket PKT_REQVAL")) Size = 10; }
  else if ( _Type == PKT_REQSETVAL )     { DBINFOL(("TxPacket::TxPacket PKT_REQSETVAL")) Size = 12; }
  else if ( _Type == PKT_REPVAL )     { DBINFOL(("TxPacket::TxPacket PKT_REPVAL")) Size = 12; }
  
  else if ( _Type == PKT_REPCONFIG )  { DBINFOL(("TxPacket::TxPacket PKT_REPCONFIG")) Size=PKB_TODECONFIG; }    // Start TodeConfig at Byte[9]
  else if ( _Type == PKT_REPVALS ) { DBINFOL(("TxPacket::TxPacket PKT_REPVALS")) Size=PKB_DITidx; }               // Start Values at Byte[9]
  
  else { DBERRORAL(("UNIDENTIFIED Packet Type : "),(_Type)) return; }

  Bytes[PKB_TO_RFH] = highByte(_ToRF);    //0 
  Bytes[PKB_TO_RFL] = lowByte(_ToRF);     //1
  //Bytes[PKB_CHANNEL] set in Send()      //2   Set in E32::Send()
  //Bytes[PKB_SECH] = highByte(SecNet);   //3   Set in Secure()
  //Bytes[PKB_SECL] = lowByte(SecNet);    //4   Set in Secure()
  Bytes[PKB_TYPE] = _Type;                //5
  //Bytes[PKB_FROM_RFH] set in Send()     //6   Set in E32::Send()
  //Bytes[PKB_FROM_RFL] set in Send()     //7   Set in E32::Send()
  Bytes[PKB_TODEVER] = _Ver;              //8

  // ------ End of Static Sets ----------
  Bytes[PKB_DITidx] = _DevDITidx;             //9
  Bytes[PKB_VALUEH] = highByte(_Value);   //10
  Bytes[PKB_VALUEL] = lowByte(_Value);    //11
}
//-----------------------------------------------------------------------------------------------------
void TxPacket::TxByte(byte _Byte) {
  
  if ( Size<58 ) { Bytes[Size] = _Byte; } 
  else {
    if ( ExtraBytes == 0 ) ExtraBytes = new ArduinoQueue<byte>(512);
    if ( ExtraBytes->isFull() ) { DBERRORL(("TxPacket::TxByte ExtraBytes->isFull()")) return; }
    ExtraBytes->enqueue(_Byte);     
  }
  DBINFOAAL(("TxPacket::TxByte(Byte)(Idx/Size): "),(_Byte,HEX),(Size))
  Size++;
}
//-----------------------------------------------------------------------------------------------------
int TxPacket::Secure() {
  int i = 0; int y = 0; int Ret = 0xFFFF; byte EESec = EEPROM.read(EMC_SECNET);
  // i = 0 to 15 (Change: 4/7/ Secure Code must be smaller than 0x7F)
  // Use top bit for Size [ Size is an int ]
  // bit: 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
  //      s0  z0  s1  z1  s2  z2  s3  z3  s4  z4  s5  z5  s6  z6  s7  s8
  while ( i<14 ) {
    bitWrite(Ret,i,bitRead(EESec,y));i++;
    bitWrite(Ret,i,bitRead(Size,y));i++;
    y++;
  }
  bitWrite(Ret,i,bitRead(Size,y));y++;i++;    // i=14, y=7 Sets bit 14 to Size-bit 7[8]
  bitWrite(Ret,i,bitRead(Size,y));            // i=15, y=8 Sets bit 15 to Size-bit 8[9]
  
  Bytes[PKB_SECH] = highByte(Ret);     //3
  Bytes[PKB_SECL] = lowByte(Ret);      //4
  return Ret;
}
//-----------------------------------------------------------------------------------------------------
void TxPacket::AddTodeConfig(int _EEAddress) {
  DBENTERAL(("TxPacket::AddTodeConfig(EEAddress): "),(_EEAddress))
  
  int iDev = 0; int iTode = 0; byte DevByte = 255;
  for ( int EAdr = _EEAddress+2; EAdr<_EEAddress+AEB_TODEALLOC ; EAdr++ ) { // +3 skip RF Address

    // TODEHEAD ( Version + TodeName10 = 11-Bytes )
    if ( iTode<AEB_TODEHEAD-2 ) {
      iTode++; 
      DBINFOAA(("TxPacket::AddTodeConfig TODEHEAD EAdr,iTode: "),(EAdr),(iTode))
      TxByte( EEPROM.read(EAdr) );   // (1-11)
    
    // ADD DEVICES
    } else {
      DevByte = EEPROM.read(EAdr);
      if ( iDev>=AEB_DEVALLOC ) { DBINFOAL(("TxPacket::AddTodeConfig Device-Begin @EEPROM"),(EAdr)) iDev=0; }
      if ( iDev==0 && DevByte==0xFF ) { 
        DBINFOAL(("TxPacket::AddTodeConfig No-Device @EEPROM"),(EAdr)) 
        EAdr=EAdr+(AEB_DEVALLOC-1); iDev=AEB_DEVALLOC; continue; 
      }
      DBINFOAA(("TxPacket::AddTodeConfig DEVICE EAdr,iDev: "),(EAdr),(iDev))
      TxByte( EEPROM.read(EAdr) );
      iDev++;
    }
  }
  DBINFOAL(("TxPacket::AddTodeConfig Size = "),(Size))
}
//-----------------------------------------------------------------------------------------------------
void TxPacket::AddValue(byte _DITidx, int _Value) {
  TxByte( _DITidx ); TxByte( highByte(_Value) ); TxByte( lowByte(_Value) );
}
//_____________________________________________________________________________________________________________________
#endif