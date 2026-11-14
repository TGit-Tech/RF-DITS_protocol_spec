#pragma once

#define DB_ERROR  1
#define DB_ENTER  1
#define DB_INFO   1
// Debug Flags and Section Active Flags
#define DBUI      1
#define DBHW      1
#define DBDB      1
#define DBL       1
#define DBH       1
#define DBS       1

#define RFSERIAL Serial3          //Should put this in Ebyte Constructor. IO-Access backplane uses Arduino 14,15 Serial3 pins.

// UIEditTode
#define RFPINS 9,8,14,15,50       //IO-Access backplane RF-Pins *M0*/8, /*M1*/9, /*ATX*/14, /*ARX*/15, /*AUX*/50

#define DISPPINS 6,7,5            //BD241S IO-Access backplane Display Pins TFT(cs, dc(rs)(cd), rst)
#define DISPLIGHTPIN 4            //IO-Access backplane Display Backlight Pin
#define DISPLIGHTACTIVE HIGH

///@defgroup DT [DT] Device-Type enumeration.
/// DITS provides 1-byte to designate Device Type definitions.
/// Never (0) as RF uses null-termination '\0' to seperate records.
/// Prevent usage of (0xF?) packet types as they help signal beginning of packets.
///@{
// 0x[7] Device Types
#define DT_ONOFF        0x7E    ///< On/Off Switching Device
#define DT_DIST         0x7A    ///< Distance Sensing Device
#define DT_STSTP3W      0x79    ///< Start Stop 3-Wire
#define DT_ANAINPUT     0x78    ///< Analog Device (Pressure, Temperature)
#define DT_ANAOUTPUT    0x77    ///< Analog Output (PWM control)
#define DT_DLSETPOINT   0x76    ///< Dev Logic Control Boundary
#define DT_DLMATH       0x75    ///< Dev Logic Compare / Math Device

#define DT_MINBOUNDARY  0x75    ///< MINIMUM BOUNDARY CHECK FOR ADD DEVICE KEYS
///@}