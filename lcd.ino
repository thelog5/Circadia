// LCD helpers (from lcd.ino, made non-blocking)

// For rainbow cycling timing
int lcdColorStep = 0;

void setupLCD() {
  lcd.begin(16, 2);
  lcd.setRGB(0, 0, 100);
}

void updateLCD(unsigned long now) {
  if (now - lastLcdChange < 1000) return;
  lastLcdChange = now;

  // TOO LOUD ALERT MODE
  if (tooLoudMode) {
    // Flash red/blue backlight
    static unsigned long lastAlertFlash = 0;
    static bool alertFlashState = false;

    if (now - lastAlertFlash > 400) {
      lastAlertFlash = now;
      alertFlashState = !alertFlashState;
    }

    if (alertFlashState) {
      lcd.setRGB(255, 0, 0);   // red
    } else {
      lcd.setRGB(0, 0, 255);   // blue
    }

    // Text: Alert! Need assistance.
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("Alert! Need");

    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("assistance.");

    return;
  }

  // --- Temp label + backlight color ---
  const float band  = 1.0;    // +/- 0.5°C is OK

  const char* tempLabel;
  if (g_tempC > ideal_temp + band) {
    tempLabel = "Hot";
    lcd.setRGB(255, 0, 0);
  } else if (g_tempC < ideal_temp - band) {
    tempLabel = "Cold";
    lcd.setRGB(0, 0, 255);
  } else {
    tempLabel = "Good";
    lcd.setRGB(0, 180, 0);
  }

  // --- Light label (from 1..4 level) ---
  const char* lightLabel;
  if (g_lightLevel <= 1) {
    lightLabel = "Dark";
  } else if (g_lightLevel >= 4) {
    lightLabel = "Brgt";
  } else {
    lightLabel = "Good";
  }

  int soundDb = soundToDb(g_soundRaw);

  // Line 0: "Temp: 25.3C Hot"
  lcd.setCursor(0, 0);
  lcd.print("                ");   // clear line
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(g_tempC, 1);          // 25.3
  lcd.print("C ");
  lcd.print(tempLabel);           // Hot / Cold / Good

  // Line 1: "Light: Bright 37dB"
  lcd.setCursor(0, 1);
  lcd.print("                ");   // clear line
  lcd.setCursor(0, 1);
  lcd.print("Light: ");
  lcd.print(lightLabel);
  lcd.print(" ");
  lcd.print(soundDb);
  lcd.print("dB");
}
