/******************************************************************************************************************//**
 * @file      rf_dits.ino
 * @brief     Example Arduino Sketch for the RF-DITS protocol on wireless-UART Ebyte E22 Transceiver.
 * @details   Main Sketch
 * @version   2629 [YYMD]
 * @bug       NA
 * @note      This is currently a WIP.  Check back soon for operational code.
 * @warning   See
 * @copyright MIT Public License
 * @todo      Wire and Test
 * @author    TGIT-TECH   WIP 2629
 **********************************************************************************************************************/
#include "rf_dits.h"        //Include the Protocol code that creates TxPackets and decodes RxPackets
#define RFSERIAL Serial1    //Arduino Serial object connected to Ebyte Radio
RadioI* RF = 0;             //Interface to the Radio

void setup() {
  RFSERIAL.begin(9600);     // Begin Ebytes Radio Communication
  RF = new RadioI();        // Initialize RF Radio Interface
}

void loop() {
  // put your main code here, to run repeatedly:
  bool bNewNode = false;

  // Receive RF Data
  if ( RF==0 ) { DBERRORL(("Sys::RFLoop RF==0")) return 0; }
  if ( !RF->PacketAvailable() ) return;                                                 // Collect Bytes till PacketAvailable
  
  // 1. Check Packet is Valid
  if (!RF->Packet->IsValid()) {DBERRORL(("!RF->Packet->IsValid()")) delete(RF->Packet); RF->Packet = 0; return; }
  DBINFOAL(("Sys::RFLoop RF->Packet->Type() = "),(RF->Packet->Type(),HEX))

  // 2. Obtain the TargetNode for the Packet
  Node* TargetNode=0;                                                                   // Packets TargetNode
  if ( RF->Packet->IsREQ() ) { TargetNode = ThisNode; }                                 // REQ packets are for ThisNode
  else {                                                                                //..IsREPly are for Remote Nodes
    TargetTode = RFTode(RF->Packet->FromRF(),false);                                    // Search if Remote Node already exists
    bNewTode = (TargetNode==0 && RF->Packet->Type()==PKT_REPCONFIG);                    // If Node doesn't exist and Packet is a Config Create New Node
    if (bNewNode) {TargetNode=RFTode(RF->Packet->FromRF(),true);}                       // Create a NewNode (,true)
  }                                                                                     // 
  if ( TargetTode==0 ) {                                                                // Check Target Node
    DBERRORAL(("Sys::RFLoop TargetTode==0 PacketType:"),(RF->Packet->Type())) 
    delete(RF->Packet); RF->Packet = 0; return; 
  }
  DBINFOAL(("Sys::RFLoop TargetTode->TodeIndex:"),(TargetTode->TodeIndex))

  //------------------------- CONFIG ( No version control )----------------------------------------------------------
  if ( RF->Packet->Type() == PKT_REPCONFIG ) {                                DBINFOL(("Sys::RFLoop PKT_REPCONFIG"))
    if ( SetupMenu != 0 && AddATode != 0 ) AddATode->Status(STSRFGOT);
    RF->Packet->SaveTodeConfig( TargetTode->EEAddress() );                    // Save the Node Configuration
    TargetTode->EELoadDevices();                                              // Reload Node
    if (bNewTode) {DelTodesList->Add(new MenuName(TargetTode->EEAddress(),+3, NAVDELTODE));}       // Add new Node to delete list
    delete(RF->Packet); RF->Packet = 0; 
    if ( CurrList == TargetTode ) CurrList->DispList(true);                   // If Node is On display refresh it.
    return;                                                                   // Exit
  }
  if ( RF->Packet->Type() == PKT_REQCONFIG ) {                                DBINFOL(("Sys::RFLoop PKT_REQCONFIG"))
    TxPacket Pkt(EEPROM.read(EMC_SECNET), PKT_REPCONFIG, 
                 RF->Packet->FromRF(), TargetTode->Version() );               // Create TxPacket
    Pkt.AddTodeConfig( TargetTode->EEAddress() );                             // Load TxPacket with Configuration
    RF->Send(&Pkt);                                                           // Send Reply
    delete(RF->Packet); RF->Packet = 0; return;                               // Exit
  }

  //-------------------------------- VERSION MATCH ------------------------------------------------------------------
  if ( TargetTode->Version() != RF->Packet->Version() ) {                     // Check Version MATCH
    DBERRORAAL(("Sys::RFLoop Node PACKET Version Mismatch(TodeVer,PktVer): "),
               (TargetTode->Version()), (RF->Packet->Version()))              // Show MISMATCH
    TxPacket Pkt(EEPROM.read(EMC_SECNET), PKT_REPCONFIG, 
                 RF->Packet->FromRF(), TargetTode->Version() );               // MISMATCH Tx Update Config
    Pkt.AddTodeConfig( TargetTode->EEAddress() );                             // Tx Add Node Config
    RF->Send(&Pkt);                                                           // Send Node Config
    delete(RF->Packet); RF->Packet = 0; return;                               // Exit
  } else {
    DBINFOL(("Sys::RFLoop() TargetTode->Version() == RF->Packet->Version()"))
  }

  //-------------------------------- SINGLE DEVICE -------------------------------------------------------------------
  if ( RF->Packet->Type() == PKT_REQSETVAL || RF->Packet->Type() == PKT_REPVAL ) {
    
    Device* TargetDev=0;
    int rfid = RF->Packet->RFID();
    if ( 0<=rfid && rfid<AEB_MAXDEVICES ) TargetDev = TargetTode->Devices[rfid];    // Get TargetDev
    if ( TargetDev==0 ) {                                                           // Check TargetDev
      DBERRORL(("Sys::RFLoop TargetDev==0"))
      delete(RF->Packet); RF->Packet = 0; return;                                   // ERROR Exit
    }

    if ( RF->Packet->Type() == PKT_REQSETVAL ) {                                       DBINFOL(("Sys::RFLoop PKT_REQSETVAL"))
      TargetDev->Value(RF->Packet->SetValue(), STSRFSET);                           // Set Device Value & Reply
      TxPacket Pkt(EEPROM.read(EMC_SECNET), PKT_REPVAL, RF->Packet->FromRF(), 
                   TargetTode->Version(), TargetDev->RFID, TargetDev->Value() );    // GOTVAL the Set Value
      RF->Send(&Pkt);                                                               // Send the Reply
      
    } else if ( RF->Packet->Type() == PKT_REPVAL ) {                                DBINFOL(("Sys::RFLoop PKT_REPVAL"))
      TargetDev->Value( RF->Packet->Value( TargetDev->RFID ), STSRFGOT );           // Set GOT Value
    } 
    delete(RF->Packet); RF->Packet = 0; return;
  }
  
  //-------------------------------- MULTI DEVICE ---------------------------------------------------------------------
  if (RF->Packet->Type() == PKT_REQVALS ) {                                         DBINFOL(("Sys::RFLoop PKT_REQVALS"))
    TxPacket Pkt(EEPROM.read(EMC_SECNET), PKT_REPVALS, RF->Packet->FromRF(), TargetTode->Version() );
    for ( int i=0; i<AEB_MAXDEVICES; i++ ) {                                        // Append every Device Value
      if ( TargetTode->Devices[i]!=0 ) {                                            // Iterate Devices[]
        if ( TargetTode->Devices[i]->RFID<AEB_MAXDEVICES ) {                        // Check Device RFID
          Pkt.AddValue(TargetTode->Devices[i]->RFID, TargetTode->Devices[i]->Value() ); }
      }
    }
    RF->Send(&Pkt);                                                                 // Send Packet
    
  } else if ( RF->Packet->Type() == PKT_REPVALS ) {                                 DBINFOL(("Sys::RFLoop PKT_REPVALS"))
    for ( int i=0; i<AEB_MAXDEVICES; i++ ) {                                        // Iterate Devices
      if ( TargetTode->Devices[i]!=0 ) {                                            // Assign Device Value
        TargetTode->Devices[i]->Value(RF->Packet->Value(TargetTode->Devices[i]->RFID),STSRFGOT);
        if ( CurrList==TargetTode ) TargetTode->Devices[i]->DisplayValue();         // Update Display
        DBINFOAL(("Sys::RFLoop PKT_REPVALS RFID: "),(TargetTode->Devices[i]->RFID))
      }
    }
  }
  // Delete Packet after Processing
  delete(RF->Packet); RF->Packet = 0;
}
