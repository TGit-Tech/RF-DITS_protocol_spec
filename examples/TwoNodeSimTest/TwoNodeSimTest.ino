/******************************************************************************************************************//**
 * @file      rf_dits.ino
 * @brief     RF-DITS protocol simulation testing with (2)DITS on one Arduino and optional RAM EEPROM replacement.
 *********************************************************************************************************************/
#include <rf_dits.h>
#define RAMEEPROM             // Use RAM as EEPROM; applies to (1) or (2) DITS Engines.
#define SWSERIALLOOP          // (2)DITS Engines looped RxData to TxData and 'ConnectTo' Function
#include "DITSConfig.h"
  
// For Wired-Loopback on one Arduino Mega
// - Wire TX1(pin18) to RX2(pin17)
// - Wire TX2(pin16) to RX1(pin19) 
#if defined(SWSERIALLOOP)
  RFDITS DITS1;
  RFDITS DITS2;
#else
  RFDITS DITS1(Serial1);
  RFDITS DITS2(Serial2);
#endif

uint32_t msTimer = 0;   
int iTestStep = 0;
char* tmpname;          // General use array for DIT get Names

// 0x[7] Device Types
#define DT_ONOFF        0x7E    ///< On/Off Switching Device
#define DT_DIST         0x7A    ///< Distance Sensing Device
#define DT_STSTP3W      0x79    ///< Start Stop 3-Wire
#define DT_ANAINPUT     0x78    ///< Analog Device (Pressure, Temperature)
// -------------------------------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------------------------------
void setup()
{
  tmpname = new char[DITS1.DITNameChar];
  Serial.begin(115200);
  Serial.println(F("RF-DITS (2)DITS Test"));

  #if defined(SWSERIALLOOP)
    DITS1.ConnectTo(DITS2);
    DITS2.ConnectTo(DITS1);
  #else
    Serial1.begin(9600);
    Serial2.begin(9600);          
  #endif

  DITS1.begin();                      // You !MUST! call begin() for DITS to work.
  DITS2.begin();  
  
  // Standard settings.
  DITS1.RadioPktMaxBytes(58);          // 58 is the default.  This can be deleted.
  DITS1.RxExpireMS(500);               // 30s is the default.  We keep small for wired-comm.
  DITS1.UpdateThisNodeName("DITS1");  // Make sure this node has a readable name.
  DITS1.UpdateThisNodeRFAddr(0x0001);  // Make sure DITS has the correct RF Address on record.
  DITS1.SecNetCode(0);                 // Assign the 'User' network security code.

  DITS2.RadioPktMaxBytes(58);
  DITS2.RxExpireMS(500);
  DITS2.UpdateThisNodeName("DITS2");
  DITS2.UpdateThisNodeRFAddr(0x0002);
  DITS2.SecNetCode(0);
}
//-------------------------------------
// Debug Displays to track Test Steps
//-------------------------------------
void DumpDITS1() {
  Serial.println(F("\nnvvvvvvvvvvvvvvvvvvvvvvvvvv RADIO1 DITS vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
  // Step 1: Iterate through Nodes
  DITSEngine::NodeDIT node = DITS1.Node();
  while (node.Next()) {
    node.NGetName(tmpname);
    Serial.print(F("NODE ["));Serial.print(node.NodeIdx());Serial.print(F("] "));
    Serial.print(tmpname);Serial.print(F(" (RF: 0x"));Serial.print(node.NRFAddr(), HEX);
    Serial.print(F(") (NDITVer: "));Serial.print(node.NDITVer());Serial.println(")");
   // Step 2: Iterate through ALL devices to find children of this node
    DITSEngine::DeviceDIT dev = DITS1.Device();
    while (dev.Next(node.NodeIdx())) {    // List Devices matching node.
      dev.DGetName(tmpname);
      Serial.print(F("  |-- DevUID: "));Serial.print(dev.DevUID());
      Serial.print(F(" | Type: "));Serial.print(dev.DevType(), HEX);
      Serial.print(F(" | Attr: 0x"));Serial.print(dev.DevAttr(), HEX);
      Serial.print(F(" | Name: "));Serial.println(tmpname);
    }
  }
  Serial.println(F("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"));
}

void DumpDITS2() {
    Serial.println(F("\nvvvvvvvvvvvvvvvvvvvvvvvvvv RADIO2 DITS vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
    // Step 1: Iterate through Nodes
    DITSEngine::NodeDIT node = DITS2.Node();
    while(node.Next()) {
      node.NGetName(tmpname);
      Serial.print(F("NODE ["));Serial.print(node.NodeIdx());Serial.print(F("] "));
      Serial.print(tmpname);Serial.print(F(" (RF: 0x"));Serial.print(node.NRFAddr(), HEX);
      Serial.print(F(") (NDITVer: "));Serial.print(node.NDITVer());Serial.println(")");
      DITSEngine::DeviceDIT dev = DITS2.Device();
      while(dev.Next(node.NodeIdx())) {     // List Devices matching node.
        dev.DGetName(tmpname);
        Serial.print(F("  |-- DevUID: "));Serial.print(dev.DevUID());
        Serial.print(F(" | Type: "));Serial.print(dev.DevType(), HEX);
        Serial.print(F(" | Attr: 0x"));Serial.print(dev.DevAttr(), HEX);
        Serial.print(F(" | Name: "));Serial.println(tmpname);
      };
    };
    Serial.println(F("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"));
}
//-------------------------------------
// Run Test Steps
//-------------------------------------
void RunTestStep() {
    iTestStep++;

    // Dump DITS on odd numbers.  Check engine changes after commands.
    if (iTestStep & 0x01) { DumpDITS1(); DumpDITS2(); }

    // Add some devices to DITS1
    if (iTestStep==2) {DITS1.AddThisNodeDevice(DT_ONOFF, DRW, "devonoff");}
    if (iTestStep==4) {DITS1.AddThisNodeDevice(DT_ANAINPUT, DRO, "devtemp");}
           
    // DITS2 Requests DITS info from DITS1.
    if (iTestStep==6) {DITS2.TxAddRemoteNode(0x0001);}

    // DITS2 does a device setting on DITS1
    if (iTestStep==8) {DITS2.TxSetRemoteDevVal(0x0001, 0, 500);}

    // DITS1 adds another device to it.
    if (iTestStep==10) {DITS1.AddThisNodeDevice(DT_STSTP3W, DRO, "runtdist");}

    // DITS2 tries to do another device setting on DITS1 but because DITS1 has changed
    // its DITS info, by added another device, the next DITS2 attempt to control it 
    // will trigger a mis-match of its 'learned' DITINFO record of DITS1 so DITS1 will send
    // DITS2 a new DITS info packet.
    if (iTestStep==12) {DITS2.TxSetRemoteDevVal(0x0001, 0, 500);}  // trigger a mis-match update (req is tossed).

    // With the new DITS info of DITS1, DITS2 can now successfully make a setting.
    if (iTestStep==14) {DITS2.TxSetRemoteDevVal(0x0001, 0, 500);}  // trigger a mis-match update (req is tossed).

    // Now repeat the mis-match process by deleting a device on DITS1.
    if (iTestStep==16) {DITS1.DelThisNodeDevice(0);}

    // Again when DITS2 tries to control DITS1 a 'learned' hardward mis-match occurs.
    // So DITS1 sends DITS2 it's 'updated' DITINFO.
    if (iTestStep==18) {DITS2.TxSetRemoteDevVal(0x0001, 0, 500);}
    
    // DITS1 adds a On/Off Read/Write Secure Set Device (to and check Secure Set Feature)
    if (iTestStep==20) {DITS1.AddThisNodeDevice(DT_ONOFF, DRWSS, "onoffSS");}

    // Again when DITS2 tries to control DITS1 a mis-match (because a device was added) is triggered.
    if (iTestStep==22) {DITS2.TxSetRemoteDevVal(0x0001, 0, 500);}  // trigger a mis-match update (req is tossed).

    // However; Once DITS2 uploads the new DITS1 DITINFO it can now control DITS1.
    // Since the Device being remotely set is an RWSS a 'Secure Set' Challenge is required.
    if (iTestStep==24) {DITS2.TxSetRemoteDevVal(0x0001, 0, 150);}  // now try to set the RWSS

    // Now we can delete the devices on DITS1
    if (iTestStep==26) {
      DITS1.DelThisNodeDevice(0);
      DITS1.DelThisNodeDevice(1);
      DITS1.DelThisNodeDevice(2);
      DITS1.DelThisNodeDevice(3);
    }

    // Repeat Test Steps
    if (iTestStep >= 26) iTestStep=0;
}
// -------------------------------------------------------------------------------------------------
// Loop
// -------------------------------------------------------------------------------------------------
void loop()
{
  if (millis()-msTimer >= 2000) { msTimer=millis(); RunTestStep();}
  DITS1.ProcessLoop();
  DITS2.ProcessLoop();
  #if !defined(SWSERIALLOOP)
    while (Serial1.available()>0) {DITS1.RxData(Serial1.read());}
    while (Serial2.available()>0) {DITS2.RxData(Serial2.read());}
  #endif
}
