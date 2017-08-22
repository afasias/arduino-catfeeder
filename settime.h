
void time_screen() {
  int prpos = -1;
  uint32_t prts = millis();
  bool overflow = false;
  lcd.clear();
  lcd.blink();
  DateTime now = rtc.now();
  int hour = now.hour(), minute = now.minute(), day = now.day(), month = now.month(), year = now.year();
  char line[17];
  int pos = 0;
  static const int y[] = { 1, 4, 7, 10, 15 };
  while (1) {
    lcd.setCursor(0,0);
    sprintf(line,"%02d:%02d %02d-%02d-%04d", hour, minute, day, month, year);
    lcd.print(line);
    lcd.setCursor(y[pos],0);
    uint32_t button = getIRButton(10000);
    waitUntilNoButton();
    int digit = digitFromButton(button);
    if (digit >= 0) {
      uint32_t val;
      switch (pos) {
        case 0:
          val = ((prpos != pos || (millis()-prts > 5000) || overflow) ? 0 : hour) * 10 + digit;
          if (! (overflow = val >= 24)) hour = val;
          break;
        case 1:
          val = ((prpos != pos || (millis()-prts > 5000) || overflow) ? 0 : minute) * 10 + digit;
          if (! (overflow = val >= 60)) minute = val;
          break;
        case 2:
          val = ((prpos != pos || (millis()-prts > 5000) || overflow) ? 0 : day) * 10 + digit;
          if (! (overflow = val > 31)) day = val;
          break;
        case 3:
          val = ((prpos != pos || (millis()-prts > 5000) || overflow) ? 0 : month) * 10 + digit;
          if (! (overflow = val > 12)) month = val;
          break;
        case 4:
          val = ((prpos != pos || (millis()-prts > 5000) || overflow) ? 0 : year) * 10 + digit;
          if (! (overflow = val > 9999)) year = val;
          break;
      }
      prpos = pos;
      prts = millis();
      continue;
    } else {
      prpos = -1;
      overflow = false;
    }
    if (button) {
      bool wrong = false;
      if (day < 1) day = 1, wrong = true;
      if (month < 1) month = 1, wrong = true;
      if (year < 2000) year = 2000, wrong = true;
      if (wrong) continue;
    }
    switch (button) {
      case BUTTON_RIGHT:
        pos = (pos + 1) % 5;
        continue;
      case BUTTON_LEFT:
        pos = (pos + 4) % 5;
        continue;
      case BUTTON_VOLUP:
        switch (pos) {
          case 0: hour = (hour+1) % 24; continue;
          case 1: minute = (minute+1) % 60; continue;
          case 2: day = day % 30 + 1; continue;
          case 3: month = month % 12 + 1; continue;
          case 4: if (year < 9999) year++; continue;
        }
        continue;
      case BUTTON_VOLDOWN:
        switch (pos) {
          case 0: hour = (hour+23) % 24; continue;
          case 1: minute = (minute+59) % 60; continue;
          case 2: day = (day+28) % 30 + 1; continue;
          case 3: month = (month+10) % 12 + 1; continue;
          case 4: if (year > 2000) year--; continue;
        }
        continue;
      case BUTTON_OK:
        rtc.adjust(DateTime(year, month, day, hour, minute, 0));
        return;
      case NO_BUTTON:
      case BUTTON_RETURN:
        return;
    }
  }
}

