#define PRINT_VAR(VAR) Serial.print(#VAR" = "),Serial.println(VAR)

#include "RTClib.h" // Uncomment this if a real DS1307 is present!

#include "config.h"
#include "rtc.h"
#include "io.h"
#include "motor.h"
#include "settime.h"
#include "timetable.h"
#include "menu.h"
#include "cron.h"

void setup() {
  Serial.begin(115200);
  rtc_init();
  io_init();
  load_timetable(); // from EEPROM
}

void loop() {
  cron();
  uint32_t button = getIRButton(100);
  switch (button) {
    case BUTTON_MENU: menu_screen(); break;
    default: if (button) Serial.println(button,HEX); break;
  }
  if (digitalRead(3) == LOW) {
    motor_manual_mode(3);
  } else if (digitalRead(5) == LOW) {
    motor_manual_mode(5);
  }
}

