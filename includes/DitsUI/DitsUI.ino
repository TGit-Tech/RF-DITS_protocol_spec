#include "DispConfig.h"
#include <Adafruit_ST7735.h>
#include <OneSlot9pt7b.h>

#define DISPPINS 6,7,5            //BD241S IO-Access backplane Display Pins TFT(cs, dc(rs)(cd), rst)
#define DISPLIGHTPIN 4            //IO-Access backplane Display Backlight Pin
#define DISPLIGHTACTIVE HIGH

Adafruit_ST7735 oDSP(DISPPINS); //TFT(cs, dc(rs)(cd), rst)

void setup() {
  pinMode(DISPLIGHTPIN, OUTPUT);
  digitalWrite(DISPLIGHTPIN, DISPLIGHTACTIVE);

  //oDSP.initR(INITR_BLACKTAB);  // Initialize ST7735
  oDSP.initR(INITR_GREENTAB);
  oDSP.setRotation(2);

  oDSP.fillScreen(ST77XX_BLACK);
  oDSP.setTextColor(ST77XX_WHITE);
  oDSP.setFont(&OneSlot9pt7b);

  oDSP.drawFastHLine(0, FONT_Y+2, TFT_WIDTH, 0x03E0);
  oDSP.drawFastVLine(TFT_VAL_X, FONT_Y+2, TFT_HEIGHT - FONT_Y, 0x03E0); // Value Divider

  oDSP.setCursor(0, TFT_ROW_Y(0));  oDSP.print("TodeName0.");
  oDSP.setCursor(0, TFT_ROW_Y(1));  oDSP.print("Device1");
  oDSP.setCursor(0, TFT_ROW_Y(2));  oDSP.print("Device2");
  oDSP.setCursor(0, TFT_ROW_Y(3));  oDSP.print("Device3");
  oDSP.setCursor(0, TFT_ROW_Y(4));  oDSP.print("Device4");
  oDSP.setCursor(0, TFT_ROW_Y(5));  oDSP.print("Device5");
  oDSP.setCursor(0, TFT_ROW_Y(6));  oDSP.print("Device6");
  oDSP.setCursor(0, TFT_ROW_Y(7));  oDSP.print("Device7");
  oDSP.setCursor(0, TFT_ROW_Y(8));  oDSP.print("Device8");
  oDSP.setCursor(0, TFT_ROW_Y(9));  oDSP.print("Canal Pump");
}

void loop() {
  // put your main code here, to run repeatedly:
/*
void MenuName::DisplayName(byte _ID) {
//DBENTERAL(("MenuName::DisplayName"),(Name()))
  if ( Display==0 ) { DBERRORL(("MenuName::DisplayName Display==0")) return; }
  Display->SETCURSOR(XPos,YPos);
  if ( NavSelected==SEL_NAME ) { Display->BGNAMESELT; Display->TXTSELT; }
  else { Display->BGNAMENORM; Display->TXTNORM; }
  if (_ID!=BNONE) Display->print(_ID);
  Display->print(Name());
}
*/
}
