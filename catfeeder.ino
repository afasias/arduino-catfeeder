#include <Wire.h>
#include "RTClib.h"
#include <Servo.h>

#define SERVO_PIN     A0

#define PRINT_VAR(XYZ)     Serial.print(#XYZ" = ");Serial.println(XYZ)

#define SERVO_MAX_FORWARD_MILLIS  1200
#define SERVO_REVERSE_MILLIS      300
#define SERVO_TREAT_MILLIS        1500

RTC_DS1307 rtc;
Servo myServo;

struct  { int h, m, d; } tbl[] = {
  {  5, 00, 4 },
  { 11, 00, 3 },
  { 17, 00, 4 },
  { 23, 00, 4 },
};

void set_motor_status(int8_t status) {
  PRINT_VAR(status);
  if (! status) {
    myServo.detach();
  } else {
    myServo.attach(SERVO_PIN);
    myServo.writeMicroseconds(1500-status*200);
  }
}

void activate_motor(int ms) {
  PRINT_VAR(ms);
  while (ms > 0) {
    int del = ms < SERVO_MAX_FORWARD_MILLIS ? ms : SERVO_MAX_FORWARD_MILLIS;
    set_motor_status(+1);
    delay(del);
    ms -= del;
    if (ms > 0) {
      set_motor_status(-1);
      delay(SERVO_REVERSE_MILLIS);
      ms += SERVO_REVERSE_MILLIS;
    }
  }
  set_motor_status(0);
}

void setup() {
  // Start serial for debugging
  Serial.begin(115200);

  // initialize rtc
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  
  // Apply 5V for DS1307 from A2-A3
  pinMode(A2,OUTPUT);
  digitalWrite(A2,LOW);
  pinMode(A3,OUTPUT);
  digitalWrite(A3,HIGH);
}

void loop() {
  static int last_activation = -1;
  DateTime now = rtc.now();
  int time = now.hour() * 100 + now.minute();
  PRINT_VAR(time);
  PRINT_VAR(now.second());
  if (time != last_activation) {
    for (int i = 0; i < (sizeof(tbl)/sizeof(tbl[0])); i++) {
      if (tbl[i].h == now.hour() && tbl[i].m == now.minute()) {
        activate_motor(tbl[i].d*1000);
        last_activation = time;
        break;
      }
    }
  }
  delay(1000);
}

