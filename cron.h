
void cron() {
  
  // periodically reset LCD screen
  static uint32_t last_lcd_reset = millis();
  if (millis() - last_lcd_reset > 60000) {
    lcd.begin(16,2);
    last_lcd_reset = millis();
  }

  // is it time to serve food?
  static int last_activation = -1;
  DateTime now = rtc.now();
  int time = now.hour() * 100 + now.minute();
  if (time != last_activation) {
    for (int i = 0; i < (sizeof(tbl)/sizeof(tbl[0])); i++) {
      if (tbl[i].h == now.hour() && tbl[i].m == now.minute()) {
        lcd.blink();
        lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Meal is being");
        lcd.setCursor(3,1);
        lcd.print("served...");
        serve_food(tbl[i].d*1000);
        last_activation = time;
        break;
      }
    }
  }
  
  // update display
  lcd.noBlink();
  char line[17];
  sprintf(line,"%02d:%02d %02d-%02d-%d", now.hour(), now.minute(), now.day(), now.month(), now.year());
  lcd.setCursor(0,0);
  lcd.print(line);
  // calculate count down
  uint32_t ds = 86400, dm, dh;
  uint32_t cur = (uint32_t)(now.hour()) * 3600 + (uint32_t)(now.minute()) * 60 + (uint32_t)(now.second());
  for (int i = 0; i < tblsize; i++) {
    uint32_t tmtbl = (uint32_t)(tbl[i].h) * 3600 + (uint32_t)(tbl[i].m) * 60;
    uint32_t dt = tmtbl > cur ? tmtbl - cur : 86400 - cur + tmtbl;
    if (dt < ds) ds = dt;
  }
  dm = ds / 60;
  ds %= 60;
  dh = dm / 60;
  dm %= 60;
  sprintf(line, "  %2luh %2lum %2lus   ", dh, dm, ds);
  lcd.setCursor(0,1);
  lcd.print(line);
}

