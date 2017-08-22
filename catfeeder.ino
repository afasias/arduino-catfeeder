#include <avr/pgmspace.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Servo.h>

#define ESP8266_RX    A0
#define ESP8266_TX    A1

#define SERVO_PIN     A2

#define PRINT_VAR(XYZ)     Serial.print(#XYZ" = ");Serial.println(XYZ)

#define COMMAND_MUX           0
#define COMMAND_SERVER        1
#define COMMAND_CLOSE         2
#define COMMAND_PREPARE_PAGE  3
#define COMMAND_SEND_PAGE     4
#define COMMAND_CONNECT_TIME  5
#define COMMAND_SEND_TIME1    6
#define COMMAND_SEND_TIME2    7
#define COMMAND_CIFSR         8

#define SERVO_MAX_FORWARD_MILLIS  1200
#define SERVO_REVERSE_MILLIS      300
#define SERVO_TREAT_MILLIS        1500

#define ESP_READ_TIMEOUT          500

/* type definitions */

struct Command {
  uint8_t id;
  uint8_t mux_id;
  Command *next;
};

struct Sched {
  uint32_t time;
  uint8_t dur;
};

/* global vars */

Servo myServo;
SoftwareSerial esp(ESP8266_RX,ESP8266_TX);
Command *commands = NULL;
Sched schedule[4];
uint8_t schedule_size = 0;
bool treat = false;
uint32_t prev_reset_ts = time_of_day();

/* buffered reading */

char *espReadLine() {
  static char buffer[128];
  static int pos = 0;
  uint32_t ts = millis();
  while ((millis() - ts) < ESP_READ_TIMEOUT) {
    while (esp.available() && (pos < (sizeof(buffer)-1))) {
      char ch = esp.read();
      if (ch != '\n' && ch != '\r') {
        buffer[pos++] = ch;
      }
      if (ch == '\n' || !(pos < (sizeof(buffer)-1))) {
        buffer[pos] = 0;
        pos = 0;
//        PRINT_VAR(buffer);
        return buffer;  
      }
    }
  }
  return NULL;
}

void readEspInput() {
  char *st;
  bool flag = false;
  while ((st = espReadLine()) != NULL) {
    PRINT_VAR(st);
    if (! strncmp(st,"+IPD,",5)) {
      int id = atoi(st+5);
      char *query = strstr(st,":GET ");
      if (query != NULL) {
        dispatchRequest(id,query+5);
        continue;
      }
      query = strstr(st,":time_of_day=");
      if (query != NULL) {
        set_time_of_day(atol(query+13));
      }
    }
  }
}

/* commands */

void queueCommand( uint8_t id, uint8_t mux_id ) {
  Command *command = new Command();
  command->id = id;
  command->mux_id = mux_id;
  command->next = NULL;
  if (commands == NULL) {
    commands = command;
  } else {
    Command *last = commands;
    while (last->next != NULL) last = last->next;
    last->next = command;
  }
}

/* time functions */

uint32_t calculate_time( uint8_t h, uint8_t m, uint8_t s ) {
  return (uint32_t)h * 3600 + m * 60 + s;
}

char time_to_str[9];
char *time_to_str_sec( uint32_t tm ) {
  int min = tm / 60;
  tm %= 60;
  int hour = min / 60;
  min %= 60;
  sprintf(time_to_str,"%02d:%02d:%02d",hour,min,tm);
  return time_to_str;
}

char *time_to_str_nosec( uint32_t tm ) {
    int min = tm / 60;
    int hour = min / 60;
    min %= 60;
    sprintf(time_to_str,"%02d:%02d",hour,min);
    return time_to_str;
}

uint32_t time_of_day_millis = 0;
uint32_t set_time_millis = 0;
uint32_t prev_time = time_of_day();

uint32_t time_of_day() {
  uint32_t ts_millis = millis();
  uint32_t dt_millis = set_time_millis > ts_millis ? 1 + set_time_millis + ~ts_millis : ts_millis - set_time_millis;
  time_of_day_millis = (time_of_day_millis + dt_millis) % 86400000;
//  PRINT_VAR(time_of_day_millis);
  set_time_millis = ts_millis;
  return time_of_day_millis/1000;
}

void set_time_of_day(uint32_t time_of_day) {
  set_time_millis = millis();
  time_of_day_millis = time_of_day * 1000;
  PRINT_VAR(time_of_day_millis);
  if (! (prev_time = time_of_day)) {
    prev_time = 86399;
    prev_reset_ts = 86399;
  }
}

/* schedule */

void store_schedule() {
  int addr = 0;
  EEPROM.put(addr,schedule_size);
  addr += sizeof(schedule_size);
  for (int i = 0; i < schedule_size; i++) {
    EEPROM.put(addr,schedule[i]);
    addr += sizeof(schedule[i]);
  }
}

void load_schedule() {
  int addr = 0;
  EEPROM.get(addr,schedule_size);
  addr += sizeof(schedule_size);
  if (schedule_size > 4) {
    schedule_size = 4;
  }
  for (int i = 0; i < schedule_size; i++) {
    EEPROM.get(addr,schedule[i]);
    schedule[i].time %= 86400;
    schedule[i].dur %= 99;
    addr += sizeof(schedule[i]);
  }
}

/* servo control */

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

void update_servo() {
  if (treat) {
    treat = false;
    servo_activate(SERVO_TREAT_MILLIS);
    return;
  }
//  PRINT_VAR(prev_time);
  uint32_t time = time_of_day();
//  PRINT_VAR(time);
  bool overflow = prev_time > time;
//  PRINT_VAR(overflow);
  for (int i = 0; i < schedule_size; i++) {
//    PRINT_VAR(i);
//    PRINT_VAR(schedule[i].time);
    if ((overflow || (prev_time < schedule[i].time)) && (schedule[i].time <= time)) {
      servo_activate(schedule[i].dur*1000);
      break;
    }
  }
  prev_time = time;
}

/* time and networking */

void update_time_and_network() {
//  PRINT_VAR(prev_time);
  uint32_t ts = time_of_day();
  uint32_t dt = prev_reset_ts > ts ? 1 + prev_reset_ts + ~ts : ts - prev_reset_ts;
  PRINT_VAR(dt);
  if (dt > 3600) {
    Serial.println("RESETING");
    queueCommand(COMMAND_SERVER,0);
    queueCommand(COMMAND_CONNECT_TIME,0);
    queueCommand(COMMAND_SEND_TIME1,0);
    queueCommand(COMMAND_SEND_TIME2,0);
    prev_reset_ts = ts;
  }
}

/* web request dispatcher */

char *decodeUrl(char *q) {
  char *cp, *cp2;
  for (cp = q, cp2 = q; *cp; cp++,cp2++) {
    if (*cp == '+') {
      *cp2 = ' ';
    } else if (*cp == '%') {
      if (cp[1] == '\0' || cp[2] == '\0') {
        break;
      } else if (cp[1] == '3' && cp[2] == 'A') {
        *cp2 = ':';
      } else if (cp[1] == '0' && (cp[2] == 'D' || cp[2] == 'A')) {
        *cp2 = ' ';
      }
      cp += 2;
    } else {
      *cp2 = *cp;
    }
  }
  *cp2 = '\0';
  return q;
}

void dispatchRequest( int id, char *query ) {
  decodeUrl(query);
  PRINT_VAR(query);
  if (strstr(query,"/favicon")) {
    queueCommand(COMMAND_CLOSE,id);
    return;
  }
  int h[4], m[4], s[1], d[4];
  int res;
  if ((res = sscanf(query,"/?time=%d:%d:%d",h,m,s)) >= 2) {
    set_time_of_day(calculate_time(h[0],m[0],res > 2 ? s[0] : 0));
  } else if ((res = sscanf(query,"/?schedule=%d:%d %d %d:%d %d %d:%d %d %d:%d %d",h+0,m+0,d+0,h+1,m+1,d+1,h+2,m+2,d+2,h+3,m+3,d+3)) >=3) {
    schedule_size = res/3;
    for (int i = 0; i < schedule_size; i++) {
      schedule[i].time = calculate_time(h[i],m[i],0);
      schedule[i].dur = d[i];
    }
    store_schedule();
  } else if (!strncmp(query,"/?treat=1",8)) {
    treat = true;
  }
  queueCommand(COMMAND_PREPARE_PAGE,id);
}

/* command dispatcher */

char resp[] =
  "<html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=2.0, maximum-scale=2.0, user-scalable=no\"></head>"
    "<body>"
      "<h3><a href=\"/\">Cat Feeder</a></h3>"
      "<form method=\"GET\">"
        "<p>Time:</p>"
        "<p><input type=\"text\" name=\"time\" value=\"@       \" maxlength=\"8\" size=\"8\"><input type=\"submit\"></p>"
      "</form>"
      "<form method=\"GET\">"
        "<p>Schedule:</p>"
        "<p><textarea name=\"schedule\" rows=\"4\" cols=\"8\">#                                               </textarea></p>"
        "<p><input type=\"submit\"></p>"
      "</form>"
      "<form method=\"GET\">"
        "<p>Treat:<input type=\"checkbox\" name=\"treat\" value=\"1\"><input type=\"submit\"></p>"
      "</form>"
    "</body>"
  "</html>";
char *timep = strchr(resp,'@');
char *tblp = strchr(resp,'#');

void dispatchTopmostCommand() {
  char *tmstr, *ptr;
  if (commands != NULL) {
    switch (commands->id) {
      case COMMAND_MUX:
        esp.write("AT+CIPMUX=1\r\n");
        break;
      case COMMAND_SERVER:
        esp.write("AT+CIPSERVER=1,80\r\n");
        break;
      case COMMAND_CIFSR:
        esp.write("AT+CIFSR\r\n");
        break;
      case COMMAND_CLOSE:
        esp.write("AT+CIPCLOSE=");
        esp.print(commands->mux_id);
        esp.write("\r\n");
        break;
      case COMMAND_PREPARE_PAGE:
        tmstr = time_to_str_sec(time_of_day());
        memcpy(timep,tmstr,8);
        memset(tblp,' ',48);
        ptr = tblp;
        for (int i = 0; i < schedule_size; i++) {
          tmstr = time_to_str_nosec(schedule[i].time);
          memcpy(ptr,tmstr,5);
          ptr += 6;
          char ch[4];
          sprintf(ch,"%d",schedule[i].dur);
          memcpy(ptr,ch,strlen(ch));
          ptr += strlen(ch);
          *ptr++ = '\n';
        }
        esp.write("AT+CIPSEND=");
        esp.print(commands->mux_id);
        esp.write(",");
        esp.print(strlen(resp));
        esp.write("\r\n");
        queueCommand(COMMAND_SEND_PAGE,commands->mux_id);
        break;
      case COMMAND_SEND_PAGE:
        esp.write(resp);
        queueCommand(COMMAND_CLOSE,commands->mux_id);
        break;
      case COMMAND_CONNECT_TIME:
        esp.write("AT+CIPSTART=0,\"TCP\",\"212.71.251.50\",80\n\n");
        break;
      case COMMAND_SEND_TIME1:
        esp.write("AT+CIPSEND=0,22\r\n");
        break;
      case COMMAND_SEND_TIME2:
        esp.write("GET /time_of_day.php\r\n");
        break;
    }
    Command *next = commands->next;
    delete commands;
    commands = next;
  }
}

/* --------------- */

void setup() {
  Serial.begin(9600);
  while (!Serial);
  load_schedule();
  esp.begin(9600);
  queueCommand(COMMAND_MUX,0);
  queueCommand(COMMAND_CONNECT_TIME,0);
  queueCommand(COMMAND_SEND_TIME1,0);
  queueCommand(COMMAND_SEND_TIME2,0);
  queueCommand(COMMAND_SERVER,0);
  queueCommand(COMMAND_CIFSR,0);
}

void loop() {
  readEspInput();
  dispatchTopmostCommand();
  update_servo();
  update_time_and_network();
}


