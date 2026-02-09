/******************************************************************************************************************//**
 * @file    EEMap.h
 * @brief   Directives for EEPROM Space Map
 *********************************************************************************************************************/
#ifndef _EEMAP_H
#define _EEMAP_H

/******************************************************************************//**
 * @defgroup AEB [A]llocation-Size of [E]EPROM in [B]ytes.
 *  (30) Devices (12-bytes)[ TYPE. ID. 10NAME ] per Tode  = 360B x 10Todes
 *  (10) Todes ( 13-bytes )[ RFH. RFL. VER. 10NAME ]      = +130B TOTAL: 3730
 *  LOCAL SETTINGS 3730 to 4096 = 366B / 30LocalDevices = 12B/ea
 * @{
 *********************************************************************************/
//#define AEB_VER 1

#define AEB_TODEALLOC   373       ///< Bytes to allocate for each Tode with Devices
#define AEB_TODEHEAD    13        ///< Bytes to allocate for each Tode Definition
#define AEB_DEVALLOC    12        ///< Bytes to allocate for each Device
#define AEB_MAXTODES    10        ///< Maximum Number of Todes
#define AEB_MAXDEVICES  30        ///< Maximum Number of Devices per Tode

#define AEB_MAX         10        ///< Maximum Default Allocation for Char[]'s
#define AEB_TODENAME    10        ///< TodeName Size to be (9)Chars + NULL
#define AEB_DEVICENAME  10        ///< Device Names to be (9)Chars + NULL

#define AEB_DEVSETTINGS 12		  ///< Storage per device ( local devices )
///@}
/******************************************************************************//**
 * @defgroup EMC [E]EPROM [M]emory [C]onstants
 * @{
 *********************************************************************************/
#define EMC_THISTODE          0
#define EMC_LOCALSETTINGS     3730  ///< Local Settings SecNet First at this address.
#define EMC_SECNET            3730
#define EMC_HDWSELECT		  3731
#define EMC_USBSERUSE		  3732
#define EMC_PINMODES		  3777	///< 0xEC1 2-bits per Pin; 18-bytes x 4 = 72 covers 70 req.
#define EMC_PINMODESET		  3768  ///< 0xEB8 1-bit flags Pin Mode Set
#define EMC_DEVSETTINGS		  3795	///< 0xED3
///@}
/******************************************************************************//**
 * @defgroup EMO [E]EPROM [M]emory [O]ffsets ( PER TODE )
 * @{
 *********************************************************************************/
#define EMO_TODERF      0     //[0][1]      2-Byte RF Address
#define EMO_TODEVER     2     //[2]         1-Byte Config Version (0~255)
#define EMO_TODENAME    3     //[3]...[12]  10-Byte Tode Name

//[DEV]ice
#define EMO_DEVTYPE     0     //[0]         1-Byte Device Type
#define EMO_DEVID       1     //[1]         1-Byte Device ID
#define EMO_DEVNAME     2     //[2]...[11]  10-Byte Device Name (AEB_DEVICENAME[10])

///@}
//_____________________________________________________________________________________________________________________
#endif
