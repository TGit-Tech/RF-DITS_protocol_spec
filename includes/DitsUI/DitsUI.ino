#include "Config.h"
#include <rf_dits.h>
#include "DitsUI.h"
#include <Adafruit_ST7735.h>
#include "DigKeyPadBD241S.h"

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
/*  This is an example using *real* persistant memory
// -------------------------------------------------------------------------------------------------
// EEPROM instance of DITSEngine Call-back Implementation via 'overloading'
// -------------------------------------------------------------------------------------------------
class EEmgrDIT : public DITSEngine {
  public:
    //DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes);
    EEmgrDIT() : DITSEngine(0,140,10) {} // 0x0800 = 2,048-bytes
  protected:
    virtual int RxReqDeviceValue(byte _DevUID) override { return 42; }
    virtual void RxReqDeviceSet(byte _DevUID, int value) override { 
        Serial.print(F("Rx Device Set on DevUID=")); Serial.print(_DevUID); 
        Serial.print(F(", to Value = ")); Serial.println(value); 
    }
    virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) override {
      // Record supplied values of a remote node.
    }
    virtual bool TxReady(uint16_t _ToRFAddr) override { return true;} //Simulation; actual radio would need checks.
    virtual void TxData(byte _TxByte) override { Serial1.write(_TxByte); }
    virtual byte pmem_read(int addr) override { return EEPROM.read(addr); }
    virtual void pmem_write(int addr, byte val) override { EEPROM.update(addr, val); }
};
*/

// -------------------------------------------------------------------------------------------------
// RAM-backed instance of DITSEngine 'Call-back' used for simulation and testing (no EEPROM wear-out)
// -------------------------------------------------------------------------------------------------
class RAMmgrDIT : public DITSEngine {
  public:
    //DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes);
    // Total Persistant Memory Allocated = MaxDITRecords * (NameFieldBytes + 4) + 1
    RAMmgrDIT(HardwareSerial& SerialPort) : DITSEngine(0,40,10), SPort(SerialPort) {memset(RAM_EEPROM, 0xFF, sizeof(RAM_EEPROM));}
  protected:
    virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) override {
      // Record supplied values of a remote node.
    }
    virtual int RxReqDeviceValue(byte ditIdx) override { return 42; }
    virtual void RxReqDeviceSet(byte _DevUID, int value) override { 
        Serial.print(F("INO.RxReqDeviceSet devUID("));Serial.print(_DevUID);Serial.print(") -to- "); Serial.print(value);
    }
    virtual bool TxReady(uint16_t _ToRFAddr) override { 
      return SPort.availableForWrite() > 58; // We know Mega UART has 64b buffer so okay to use this.
    } //Simulation; actual radio would need checks.
    virtual void TxData(byte _TxByte) override { SPort.write(_TxByte); }
    virtual byte pmem_read(int addr) override { return RAM_EEPROM[addr]; }
    virtual void pmem_write(int addr, byte val) override { RAM_EEPROM[addr] = val; }
    virtual void RxDITUpdate(byte nodeIdx) override {
      // A call-back to notify implementer Rx has changed the DIT records for 'nodeIdx'.
      // This is useful for keeping a display updated with new changes occuring behind the scenes through Rx communication.
      // Note:  This is only called via Rx.  Implementer DIT changes do not trigger this call-back
    }
  private: 
    byte RAM_EEPROM[0x0300];
    HardwareSerial& SPort;
};

//EEmgrDIT  Radio1;
RAMmgrDIT Radio1(Serial1);
RAMmgrDIT Radio2(Serial2);

//UI
Adafruit_ST7735 oDSP(DISPPINS); //TFT(cs, dc(rs)(cd), rst)
DitsUI dUI;
KeyPad* ButtonCapture = nullptr;

void setup() {
  
  Serial.begin(115200);
  Serial.println(F("DitsUI Test"));

  Radio1.begin();               // Start RF-DITS protocol on sim Radio1
  Radio2.begin();               // Start RF-DITS protocol on sim Radio2
  Serial1.begin(9600);          // Start Radio Communications for Radio1
  Serial2.begin(9600);          // Start Radio Communications for Radio2

  oDSP.initR(INITR_GREENTAB);   //Initialize ST7735 of the GFX
  
  
  ButtonCapture = new KeyPad(/*Debounce*/300, /*HoldRepeat*/2000, /*BuzzPin*/0xFF);
  ButtonCapture->StartCapture();

  // Standard RF-DITS protocol settings.
  Radio1.RadioPktMaxBytes(58);          // 58 is the default.  This can be deleted.
  Radio1.RxExpireMS(500);               // 30s is the default.  We keep small for wired-comm.
  Radio1.UpdateThisNodeName("Radio1");  // Make sure this node has a readable name.
  Radio1.UpdateThisNodeRFAddr(0x0001);  // Make sure DITS has the correct RF Address on record.
  Radio1.SecNetCode(0);                 // Assign the 'User' network security code.

  Radio2.RadioPktMaxBytes(58);
  Radio2.RxExpireMS(500);
  Radio2.UpdateThisNodeName("Radio2");
  Radio2.UpdateThisNodeRFAddr(0x0002);
  Radio2.SecNetCode(0);

  // Add some devices to see
  Radio1.AddThisNodeDevice(DT_ONOFF, DRW, "devonoff");
  Radio1.AddThisNodeDevice(DT_ANAINPUT, DRO, "devtemp");

  dUI.Begin(Radio1, oDSP);      //Start DitsUI

}

void loop() {
  // put your main code here, to run repeatedly:
  int iButton = ButtonCapture->Button();
  if(iButton!=NAVKEYNONE) {
    //Serial.println("Button Pressed.");
    dUI.ButtonPress(iButton);
  }
}
