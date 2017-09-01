
#include <IRLib.h>
#include <LiquidCrystal.h>

#include "samsung.h"
//#include "lg.h"

#define NO_BUTTON           0

IRrecv irrecv(2);
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

uint32_t getIRButton() {
  static IRdecode decoder;
  int button = NO_BUTTON;
  if (irrecv.GetResults(&decoder)) {
    decoder.decode();
    button = decoder.value;
    irrecv.resume();
  }
  return button;
}

uint32_t getIRButton( int timeout ) {
  uint32_t ts = millis();
  uint32_t button;
  while (millis()-ts < timeout && !(button = getIRButton()));
  return button;
}

void waitUntilNoButton() {
  uint32_t ts = millis();
  while (millis()-ts < 150) if (getIRButton()) ts = millis();
}

int digitFromButton( uint32_t button ) {
  switch (button) {
    case BUTTON_0: return 0;
    case BUTTON_1: return 1;
    case BUTTON_2: return 2;
    case BUTTON_3: return 3;
    case BUTTON_4: return 4;
    case BUTTON_5: return 5;
    case BUTTON_6: return 6;
    case BUTTON_7: return 7;
    case BUTTON_8: return 8;
    case BUTTON_9: return 9;
    default: return -1;
  }
}

void io_init() {
  irrecv.enableIRIn();
  lcd.begin(16, 2);
  lcd.clear();
  // pin 4 will be used as ground
  pinMode(4,OUTPUT);
  digitalWrite(4,LOW);
  // pins 3 and 5 will be used to manually move the motor back and forth
  pinMode(3,INPUT_PULLUP);
  pinMode(5,INPUT_PULLUP);
}

