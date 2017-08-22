
#ifndef _RTCLIB_H_

// RTC_DS1307 emulator

class DateTime {
  private:
    uint32_t h, m, s, y, M, d;
    void calc() {
      s = (m = ts / 60, ts) % 60;
      m = (h = m / 60, m) % 60;
      h = (d = h / 24, h) % 24;
      d = (M = d / 31, d) % 31 + 1;
      M = (y = M /12 + 2000, M) % 12 + 1;
    }
  public:
    DateTime( uint32_t y, uint32_t M, uint32_t d, uint32_t h, uint32_t m, uint32_t s ) {
      ts = (((((y-2000)*12+m-1)*31+d-1)*24+h)*60+m)*60+s;
    }
    DateTime( uint32_t ts ) {
      this->ts = ts;
    }
    DateTime() {
      uint32_t h,m,s;
      sscanf(__TIME__,"%lu:%lu:%lu",&h,&m,&s);
      ts = (h*60+m)*60+s;
    }
    int hour() { calc(); return h; }
    int minute() { calc(); return m; }
    int second() { calc(); return s; }
    int year() { calc(); return y; }
    int month() { calc(); return M; }
    int day() { calc(); return d; }
    uint32_t ts;
};

class RTC_DS1307 {
  private:
    DateTime dt;
    uint32_t ts = millis();
  public:
    DateTime now() {
      DateTime now = DateTime(dt.ts+(millis()/1000-ts));
      return now;
    }
    void adjust( DateTime dt ) {
      this->dt = dt;
      this->ts = millis();
    }
    bool begin() {
      return true;
    }
  
};

#endif

RTC_DS1307 rtc;

void rtc_init() {
  pinMode(A2,OUTPUT); // 5 Volts for DS1307 from pins A2,A3
  digitalWrite(A2,LOW);
  pinMode(A3,OUTPUT);
  digitalWrite(A3,HIGH);

 if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
}

