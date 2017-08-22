
#include <EEPROM.h>

struct tblentry { int h, m, d; } tbl[MAX_TABLE_SIZE];
int tblsize = 0;

void sort_timetable() {
  if (tblsize) {
    bool swapped;
    do {
      swapped = false;
      for (int i = 0; i < tblsize-1; i++) {
        if (tbl[i].h*60+tbl[i].m > tbl[i+1].h*60+tbl[i+1].m) {
          tblentry tmp = tbl[i];
          tbl[i] = tbl[i+1];
          tbl[i+1] = tmp;
          swapped = true;
        }
      }
    } while (swapped);
  }
}

void save_timetable() {
  int addr = 0;
  EEPROM.put(addr,tblsize);
  addr += sizeof(tblsize);
  for (int i = 0; i < tblsize; i++) {
    EEPROM.put(addr,tbl[i]);
    addr += sizeof(tblentry);
  }
}

void load_timetable() {
  int addr = 0;
  EEPROM.get(addr,tblsize);
  addr += sizeof(tblsize);
  if (tblsize > MAX_TABLE_SIZE) {
    tblsize = MAX_TABLE_SIZE;
  }
  for (int i = 0; i < tblsize; i++) {
    EEPROM.get(addr,tbl[i]);
    tbl[i].h %= 24;
    tbl[i].m %= 60;
    if (tbl[i].d > 6) {
      tbl[i].d = 6;
    } else if (tbl[i].d < 1) {
      tbl[i].d = 1;
    }
    addr += sizeof(tbl[i]);
  }
}

void print_timetable_entry( int pos, int y ) {
  lcd.setCursor(0,y);
  if (pos < tblsize) {
    char line[17];
    tblentry *ent = tbl + pos;
    static const char *bar = "******     ";
    sprintf(line,"%2d) %2d:%02d ", pos+1, ent->h, ent->m);
    strncpy(line+10,bar+(6-ent->d),6);
    lcd.print(line);
  } else {
    lcd.print("                ");
  }
}

void timetable_screen() {
  int prpos = -1, prcur = -1;
  uint32_t prts = millis();
  bool overflow = false;
  int pos = 0, cur = 0, vport = 0;
  static const int y[] = { 5, 8, 10 };
  while (1) {
    if (tblsize > 0) {
      print_timetable_entry(vport,0);
      print_timetable_entry(vport+1,1);
      lcd.setCursor(y[pos],cur-vport);
      lcd.blink();
    } else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Time table empty");
      lcd.noBlink();
    }
    uint32_t button = getIRButton(10000);
    waitUntilNoButton();
    int digit = digitFromButton(button);
    if (digit >= 0) {
      tblentry *ent = tbl + cur;
      int val;
      switch (pos) {
        case 0:
          val = ((prpos != pos || cur != prcur || (millis()-prts > 5000) || overflow) ? 0 : ent->h) * 10 + digit;
          if (! (overflow = val >= 24)) ent->h = val;
          break;
        case 1:
          val = ((prpos != pos || cur != prcur || (millis()-prts > 5000) || overflow) ? 0 : ent->m) * 10 + digit;
          if (! (overflow = val >= 60)) ent->m = val;
          break;
        case 2:
          if (digit < 2) {
            ent->d = 1;
          } else if (digit > 5) {
            ent->d = 6;
          } else {
            ent->d = digit;
          }
          overflow = false;
          break;
      }
      prpos = pos;
      prcur = cur;
      prts = millis();
      continue;
    } else {
      prpos = prcur = -1;
      overflow = false;
    }
    switch (button) {
      case BUTTON_RIGHT:
        pos = (pos + 1) % 3;
        continue;
      case BUTTON_LEFT:
        pos = (pos + 2) % 3;
        continue;
      case BUTTON_UP:
        if (cur > 0) {
          if (--cur < vport) {
            vport--;
          }
        }
        continue;
      case BUTTON_DOWN:
        if (cur < tblsize-1) {
          if (++cur > vport+1) {
            vport++;
          }
        }
        continue;
      case BUTTON_VOLUP:
        if (tblsize > 0) {
          tblentry *ent = tbl + cur;
          switch (pos) {
            case 0: ent->h = (ent->h+1) % 24; continue;
            case 1: ent->m = (ent->m+1) % 60; continue;
            case 2: if (ent->d < 6) ent->d++; continue;
          }
        }
        continue;
      case BUTTON_VOLDOWN:
        if (tblsize > 0) {
          tblentry *ent = tbl + cur;
          switch (pos) {
            case 0: ent->h = (ent->h+23) % 24; continue;
            case 1: ent->m = (ent->m+59) % 60; continue;
            case 2: if (ent->d > 1) ent->d--; continue;
          }
        }
        continue;
      case BUTTON_RED:
        if (tblsize > 0) {
          for (int i = cur; i < tblsize-1; i++) {
            tbl[i] = tbl[i+1];
          }
          tblsize--;
          if (cur >= tblsize) {
            cur = tblsize-1;
          }
          if (cur < vport) {
            vport = cur;
          }
        }
        continue;
      case BUTTON_GREEN:
        if (tblsize < MAX_TABLE_SIZE) {
          tbl[tblsize].h = 12;
          tbl[tblsize].m = 0;
          tbl[tblsize].d = 1;
          cur = tblsize++;
          if (cur > vport+1) {
            vport = cur-1;
          }
          pos = 0;
        } 
        continue;
      case BUTTON_OK:
        sort_timetable();
        save_timetable();
        return;
      case NO_BUTTON:
      case BUTTON_RETURN:
        load_timetable();
        return;
    }
  }
}

