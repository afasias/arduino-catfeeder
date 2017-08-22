
#define MENU_SIZE 4

void print_menu_entry( int pos, int y ) {
  static const char *menu[MENU_SIZE] = { "Serve treat", "Set date/time", "Time table", "Exit menu" };
  char line[17];
  sprintf(line,"%d) %s", pos+1, menu[pos]);
  for (int i = strlen(line); i < 16; i++) line[i] = ' ';
  lcd.setCursor(0,y);
  lcd.print(line);
}

void menu_screen() {
  lcd.blink();
  int cur = 0, vport = 0;
  while (1) {
    tblentry *ent = tbl + cur;
    print_menu_entry(vport,0);
    print_menu_entry(vport+1,1);
    lcd.setCursor(3,cur-vport);
    uint32_t button = getIRButton(10000);
    waitUntilNoButton();
    switch (button) {
      case BUTTON_UP:
        if (cur > 0) {
          if (--cur < vport) {
            vport--;
          }
        }
        continue;
      case BUTTON_DOWN:
        if (cur < MENU_SIZE-1) {
          if (++cur > vport+1) {
            vport++;
          }
        }
        continue;
      case BUTTON_OK:
        switch (cur) {
          case 0: serve_treat(); continue;
          case 1: time_screen(); continue;
          case 2: timetable_screen(); continue;
          case 3: return;
          default: continue;
        }
      case BUTTON_1: serve_treat(); continue;
      case BUTTON_2: time_screen(); continue;
      case BUTTON_3: timetable_screen(); continue;
      case BUTTON_4:
      case NO_BUTTON:
      case BUTTON_RETURN:
        return;
    }
  }
}


