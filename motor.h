
#include <Servo.h>

#define SERVO_MOTOR_PIN   A0

Servo myServo;

void start_motor() {
    myServo.attach(SERVO_MOTOR_PIN);
    myServo.writeMicroseconds(1300);
}

void start_motor_reverse() {
    myServo.attach(SERVO_MOTOR_PIN);
    myServo.writeMicroseconds(1700);
}

void stop_motor() {
    myServo.detach();
}

void serve_food(int ms) {
  while (ms > 0) {
    int del = ms < SERVO_MAX_FORWARD_MILLIS ? ms : SERVO_MAX_FORWARD_MILLIS;
    start_motor();
    delay(del);
    ms -= del;
    if (ms > 0) {
      start_motor_reverse();
      delay(SERVO_REVERSE_MILLIS);
      ms += SERVO_REVERSE_MILLIS;
    }
  }
  stop_motor();
}

void serve_treat() {
  lcd.blink();
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Treat is being");
  lcd.setCursor(3,1);
  lcd.print("served...");
  serve_food(TREAT_SERVING_MILLIS);
}

