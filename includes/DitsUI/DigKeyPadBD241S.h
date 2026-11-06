/**********************************************************************************************//**
 * @file    DigKeyPad.h
 * @brief   Digital Keypad Button Capture using Arduino Interrupts.
 * @authors<pre>
 *  TGit-Tech           06/2023     Original
 *	TGit-Tech			11/2023		Display Backlight Setup and PinMode in Constructor
 *  </pre>
 * @todo
 *************************************************************************************************/
#pragma once

//#define BUZZER
//#include "../config.h"
#include <Arduino.h>
#include <TGTDB.h>

/******************************************************************************//**
* @defgroup NAVKEY Navigate() Keypad Constants
* @{
*********************************************************************************/
// 0x[F]-Key Presses
#define NAVKEYNONE        0xFF
#define NAVKEYRIGHT       0xFE
#define NAVKEYLEFT        0xFD
#define NAVKEYUP          0xFC
#define NAVKEYDOWN        0xFB
#define NAVKEYSET         0xFA
#define NAVKEYOKAY        0xF9
#define KEYPRESSMIN       0xF0
///@}

//#####################################################################################################
namespace ButtonISR {
  unsigned long Debounce = 150;                   // Debounce button milliseconds
  volatile int ButtonBuffer = NAVKEYNONE;            // Store last button pressed for processing
  volatile int Current_Button = NAVKEYNONE;          // Stores last ISR Button
  volatile unsigned long Button_Press_Time = 0;   // Tracks if button is held-down

  void LeftCapture() {
	  if (ButtonBuffer != NAVKEYNONE || millis() - Button_Press_Time < Debounce) return;
	  cli();
	  Button_Press_Time = millis();                           // Record Press Time for Debounce
	  Current_Button = NAVKEYLEFT;
	  ButtonBuffer = Current_Button;
	  sei();
  }
  void DownCapture() {
	  if (ButtonBuffer != NAVKEYNONE || millis() - Button_Press_Time < Debounce) return;
	  cli();
	  Button_Press_Time = millis();                           // Record Press Time for Debounce
	  Current_Button = NAVKEYDOWN;
	  ButtonBuffer = Current_Button;
	  sei();
  }

  //-------------------------------------------------------------------------------------
  // Setup ISR
  //-------------------------------------------------------------------------------------
  void EnableInterrupt() {
    cli();                          // disable interrupt
    
	  PCMSK0=0b11110000;
	  //PCMSK0 |= (1 << PCINT4);		// Use PCINT4 = PB4 pin 10(UP).
	  //PCMSK0 |= (1 << PCINT5);		// Use PCINT5 = PB5 pin 11(RHT).
	  //PCMSK0 |= (1 << PCINT6);		// Use PCINT6 = PB6 pin 12(OK).
	  //PCMSK0 |= (1 << PCINT7);		// Use PCINT7 = PB7 pin 13(SET).
	  PCICR |= (1 << PCIE0);		// Enable (PCICR) Pin Change Interrupt Control Register (see page 91 of the Datasheet)
	  attachInterrupt(digitalPinToInterrupt(2), ButtonISR::LeftCapture, FALLING);
	  attachInterrupt(digitalPinToInterrupt(3), ButtonISR::DownCapture, FALLING);
    sei();  //allows interrupts
  }

  ISR(PCINT0_vect) {
	  if (ButtonBuffer != NAVKEYNONE || millis() - Button_Press_Time < Debounce) return;
	  cli();
    Button_Press_Time = millis();		// Record Press Time for Debounce
    Current_Button = NAVKEYNONE;		// Default is NAVKEYNONE
	  if (     (PINB&0b11110000)==0b10110000) { Current_Button = NAVKEYSET; }
	  else if ((PINB&0b11110000)==0b01110000) { Current_Button = NAVKEYOKAY; }
	  else if ((PINB&0b11110000)==0b11100000) { Current_Button = NAVKEYRIGHT; }
	  else if ((PINB&0b11110000)==0b11010000) { Current_Button = NAVKEYUP; }
	  //Serial.print("Current_Button=");//Serial.println(Current_Button,HEX);
	  //Serial.println(PINB&0b11110000,BIN);
	  //KEY  PIN            BIT  SHOULD-BE     IS
	  //ST = D12 = PORT_PINB_6 = 1011_0000	0011_0000  [7] is LOW, should be HIGH  1011-0000
	  //OK = D13 = PORT PINB_7 = 0111_0000	0011_0000  0111-0000 0111-0000
	  //RT = D10 = PORT PINB_4 = 1110_0000	0110_0000  [7] is LOW, should be HIGH  1110_0000 0110-0000
	  //UP = D11 = PORT PINB_5 = 1101_0000    0101_0000  [7] is LOW, should be HIGH  1101_0000 0101-0000 0101-0000
	  //DW = D3 = INT5
	  //LT = D2 = INT4
	  ButtonBuffer = Current_Button;
	  sei();
  }
}
//#####################################################################################################
/**********************************************************************************************//**
 * @class   KeyPad
 * @brief   DigKeyPad.h
 *************************************************************************************************/
class KeyPad {
  public:
    // Constructors for parameterized Base Class
    KeyPad(unsigned long _ButtonDebounce_ms = 150, unsigned long _HoldRepeat = 2000, uint8_t _BuzzerPin = 0xFF) {
		  DBINFO(("KeyPad::KeyPad DIGITAL\n"))
		
		  pinMode(2, INPUT_PULLUP);
		  pinMode(3, INPUT_PULLUP);
		  pinMode(10, INPUT_PULLUP); //PB4
		  pinMode(11, INPUT_PULLUP);  //PB5
		  pinMode(12, INPUT_PULLUP);  //PB6
		  pinMode(13, INPUT_PULLUP);  //PB7
		
		  ButtonISR::Debounce = _ButtonDebounce_ms;
		  Hold_Repeat = _HoldRepeat;
		  BuzzerPin = _BuzzerPin;
	  }

    // Public Functions
    void StartCapture() {
		  DBINFO(("KeyPad::StartCapture() DIGITAL\n"))
		  ButtonISR::EnableInterrupt();
	  }
    int Button(){
		  int Rtn = NAVKEYNONE;                                         // Default return NONE
		  if ( ButtonISR::ButtonBuffer != NAVKEYNONE ) {                    // Check Button buffer for a button press
			  Rtn = ButtonISR::ButtonBuffer;
			  #if defined(BUZZER)
			  if ( BuzzerPin != 0xFF ) {
				  noTone(BuzzerPin);
				  tone(BuzzerPin,1200,15);
			  }
			  #endif
			  ButtonISR::ButtonBuffer = NAVKEYNONE;                           // Clear the Buffer
		  }
		  return Rtn;
	  }
	
  private:
    unsigned long Last_Button_ms = 0;
    unsigned long Debounce = 150;
    unsigned long Hold_Repeat = 2000;
    int Last_Button_Retrieved = NAVKEYNONE;
    uint8_t BuzzerPin = 0xFF;
}; 
//_____________________________________________________________________________________________________________________