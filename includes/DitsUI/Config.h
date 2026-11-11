#define DB_ERROR 1
#define DB_ENTER 1
#define DB_INFO 1
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

// 1.8in ST7735S TFT is 128x160
#define TFT_WIDTH   128
#define TFT_HEIGHT  160


#define FONT_X      8           // Chosen Font xAdvance per character
#define FONT_Y      16          // Chosen Font yAdvance per line
#define TFT_VAL_X   100