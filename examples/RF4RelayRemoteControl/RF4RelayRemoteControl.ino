/******************************************************************************************************************//**
 * @file      RF4RelayRemoteControl.ino
 * @brief     RF-DITS protocol Controlling 4-Relays by Remote Control Example.
 *********************************************************************************************************************/
#include <rf_dits.h>
#define RAMEEPROM             // Use RAM as EEPROM; applies to (1) or (2) DITS Engines.
//#define SWSERIALLOOP          // (2)DITS Engines looped RxData to TxData and 'ConnectTo' Function
#include "DITSConfig.h"

#define RELAYNODE     // On Compile: Comment out for Handheld.  Leave for Node connected to Relays.

#define RFSERIAL Serial3
RFDITS DITS(RFSERIAL);

uint32_t msTimer = 0;   
int iTestStep = 0;
char* tmpname;          // General use array for DIT get Names

// 0x[7] Device Types
#define DT_ONOFF        0x7E    ///< On/Off Switching Device
#define DT_DIST         0x7A    ///< Distance Sensing Device
#define DT_STSTP3W      0x79    ///< Start Stop 3-Wire
#define DT_ANAINPUT     0x78    ///< Analog Device (Pressure, Temperature)

uint8_t Relay_devUID[4];
// -------------------------------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------------------------------
void setup()
{
  tmpname = new char[DITS.DITNameChar];
  Serial.begin(115200);
  Serial.println(F("RF-DITS 4-Relay Remote Controlled Test"));

  RFSERIAL.begin(9600);
  DITS.begin();                      // You !MUST! call begin() for DITS to work.
  
  #if defined(RELAYNODE)
    DITS.RadioPktMaxBytes(58);          // 58 is the default.  This can be deleted.
    DITS.RxExpireMS(500);               // 30s is the default.  We keep small for wired-comm.
    DITS.UpdateThisNodeName("Relays");  // Make sure this node has a readable name.
    DITS.UpdateThisNodeRFAddr(0x0001);  // Make sure DITS has the correct RF Address on record.
    DITS.SecNetCode(0);                 // Assign the 'User' network security code.

    Relay_devUID[0] = DITS.AddThisNodeDevice(DT_ONOFF, 0x00, "Relay1");
    Relay_devUID[1] = DITS.AddThisNodeDevice(DT_ONOFF, 0x00, "Relay2");
    Relay_devUID[2] = DITS.AddThisNodeDevice(DT_ONOFF, 0x00, "Relay3");
    Relay_devUID[3] = DITS.AddThisNodeDevice(DT_ONOFF, 0x00, "Relay4");
  #else
    DITS2.RadioPktMaxBytes(58);
    DITS2.RxExpireMS(500);
    DITS2.UpdateThisNodeName("Handheld");
    DITS2.UpdateThisNodeRFAddr(0x0002);
    DITS2.SecNetCode(0);
  #endif
}
//-------------------------------------
// Debug Displays to track Test Steps
//-------------------------------------
void DumpDITS() {
  Serial.println(F("\nnvvvvvvvvvvvvvvvvvvvvvvvvvv RADIO1 DITS vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
  // Step 1: Iterate through Nodes
  DITSEngine::NodeDIT node = DITS.Node();
  while (node.Next()) {
    node.NGetName(tmpname);
    Serial.print(F("NODE ["));Serial.print(node.NodeIdx());Serial.print(F("] "));
    Serial.print(tmpname);Serial.print(F(" (RF: 0x"));Serial.print(node.NRFAddr(), HEX);
    Serial.print(F(") (NDITVer: "));Serial.print(node.NDITVer());Serial.println(")");
   // Step 2: Iterate through ALL devices to find children of this node
    DITSEngine::DeviceDIT dev = DITS.Device();
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

//-------------------------------------
// Run Test Steps
//-------------------------------------
void RunTestStep() {
    iTestStep++;

    // Dump DITS on odd numbers.  Check engine changes after commands.
    if (iTestStep & 0x01) { DumpDITS(); }

  #if !defined(RELAYNODE)           
    // Requests DITS info from RELAYNODE.
    if (iTestStep==6) {DITS.TxAddRemoteNode(0x0001);}

    // Do a device setting on RELAYNODE
    if (iTestStep==8) {DITS.TxSetRemoteDevVal(0x0001, 0, 1);}
  #endif

    // Repeat Test Steps
    if (iTestStep >= 26) iTestStep=0;
}
// -------------------------------------------------------------------------------------------------
// Loop
// -------------------------------------------------------------------------------------------------
void loop()
{
  if (millis()-msTimer >= 2000) { msTimer=millis(); RunTestStep();}
  DITS.ProcessLoop();
}
