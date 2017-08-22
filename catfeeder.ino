#include <EEPROM.h>
#include <Servo.h>

#define SERVO_PIN     A2

#define PRINT_VAR(XYZ)     Serial.print(#XYZ" = ");Serial.println(XYZ)

#define MAX_TIME_TABLE_ENTRIES    10
#define SERVO_MAX_FORWARD_MILLIS  1200
#define SERVO_REVERSE_MILLIS      300
#define SERVO_TREAT_MILLIS        1500

struct time_table_entry {
  int hour;
  int minute;
  int duration;
};

Servo myServo;
time_table_entry time_table[MAX_TIME_TABLE_ENTRIES];
int time_table_entries = 0;

int set_time = 0;
uint32_t set_time_millis = 0;

int time_of_day() {
  uint32_t ts_millis = millis();
  uint32_t dt_millis = set_time_millis > ts_millis ? 1 + set_time_millis + ~ts_millis : ts_millis - set_time_millis;
  int time_of_day = (set_time + dt_millis/60000) % 1440;
  if (dt_millis > 86400000) {
    set_time = time_of_day;
    set_time_millis = ts_millis;
  }
  return time_of_day;
}

void set_time_of_day(int time_of_day) {
  set_time_millis = millis();
  set_time = time_of_day;
}

void save_time_table() {
  int addr = 0;
  EEPROM.put(addr,time_table_entries);
  addr += sizeof(time_table_entries);
  for (int i = 0; i < time_table_entries; i++) {
    EEPROM.put(addr,time_table[i]);
    addr += sizeof(time_table[i]);
  }
}

void load_time_table() {
  int addr = 0;
  EEPROM.get(addr,time_table_entries);
  addr += sizeof(time_table_entries);
  if (time_table_entries > MAX_TIME_TABLE_ENTRIES) {
    time_table_entries = MAX_TIME_TABLE_ENTRIES;
  }
  for (int i = 0; i < time_table_entries; i++) {
    EEPROM.get(addr,time_table[i]);
    time_table[i].hour %= 24;
    time_table[i].minute %= 60;
    time_table[i].duration %= 10;
    addr += sizeof(time_table[i]);
  }

}

void servo_control(int8_t status) {
  if (! status) {
    Serial.println("STOPPING THE MOTOR");
    myServo.detach();
  } else {
    Serial.println(status > 0 ? "MOTOR FORWARD" : "MOTOR REVERSE");
    myServo.attach(SERVO_PIN);
    myServo.writeMicroseconds(1500-status*200);
  }
}

void servo_activate(int ms) {
  Serial.print("SERVO ACTIVATE FOR: ");
  Serial.print(ms);
  Serial.println(" milliseconds total");
  while (ms > 0) {
    int del = ms < SERVO_MAX_FORWARD_MILLIS ? ms : SERVO_MAX_FORWARD_MILLIS;
    servo_control(+1);
    delay(del);
    ms -= del;
    if (ms > 0) {
      servo_control(-1);
      delay(SERVO_REVERSE_MILLIS);
      ms += SERVO_REVERSE_MILLIS;
    }
  }
  servo_control(0);
}

void do_servo() {
  static int last_activation = -1;
  int time = time_of_day();
  if (time == last_activation) {
    return;
  }
  int h = time / 60;
  int m = time % 60;
  for (int i = 0; i < time_table_entries; i++) {
    if (time_table[i].hour == h && time_table[i].minute == m) {
      servo_activate(time_table[i].duration*1000);
      last_activation = time;
      break;
    }
  }
}

void print_help() {
  Serial.println("Command set");
  Serial.println("-----------");
  Serial.println("t           : Display time");
  Serial.println("t HH:MM     : Set time");
  Serial.println("l           : List time table");
  Serial.println("a HH:MM DUR : Add ne time table entry");
  Serial.println("clear       : Clear time table");
  Serial.println("save        : Save changes");
  Serial.println("discard     : Discard changes");
  Serial.println("treat       : Serve a treat");
}

void print_time_table() {
  if (time_table_entries > 0) {
    Serial.println("Time table:");
    char st[32];
    for (int i = 0; i < time_table_entries; i++) {
      time_table_entry *en = time_table + i;
      sprintf(st,"%2d:%02d -> %d sec", en->hour, en->minute, en->duration );
      Serial.println(st);
    }
  } else {
    Serial.println("Time table empty.");
  }
}

void print_time_of_day() {
  char st[16];
  int time = time_of_day();
  sprintf(st,"%d:%02d",  time / 60, time % 60);
  Serial.println(st);
}
void do_serial() {
  if (Serial.available()) {
    String st = Serial.readString();
    st.trim();
    if (st == "l") {
      print_time_table();
    } else if (st == "treat") {
      Serial.println("STARTING TREAT...");
      servo_activate(SERVO_TREAT_MILLIS);
      Serial.println("Done.");
    } else if (st == "clear") {
      time_table_entries = 0;
      print_time_table();
    } else if (st == "save") {
      save_time_table();
      Serial.println("Saved time table into EEPROM");
    } else if (st == "discard") {
      load_time_table();
      print_time_table();
    } else if (st.startsWith("a ")) {
      int h, m, d;
      if (sscanf(st.c_str()+2,"%d:%d %d",&h,&m,&d) == 3) {
        if (time_table_entries >= MAX_TIME_TABLE_ENTRIES) {
          Serial.println("Too many entries!");
        } else {
          time_table[time_table_entries].hour = h;
          time_table[time_table_entries].minute = m;
          time_table[time_table_entries].duration = d;
          time_table_entries++;
          print_time_table();
        }
      } else {
          print_help();
      }
    } else if (st == "t") {
      print_time_of_day();
    } else if (st.startsWith("t ")) {
      int h, m;
      if (sscanf(st.c_str()+2,"%d:%d",&h,&m) == 2) {
        set_time_of_day(h*60+m);
        Serial.print("New time: ");
        print_time_of_day();
      } else {
          print_help();
      }
    } else {
      Serial.print("Unknown command: ");
      Serial.println(st);
      print_help();
    }
  }
}

void wait_for_serial_data() {
  pinMode(13, OUTPUT);
  digitalWrite(13,HIGH);
  uint32_t ts = millis();
  uint8_t status = HIGH;
  while (!Serial.available()) {
    if ((millis()-ts) > 500) {
      digitalWrite(13,status ^= 1);
      ts = millis();
    }
    do_servo();
  }
  digitalWrite(13,LOW);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.setTimeout(100);
  load_time_table();
  wait_for_serial_data();
}

void loop() {
  do_serial();
  do_servo();
}


