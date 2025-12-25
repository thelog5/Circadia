// ==== WS2812B Sleep-Safe Amber–Red Glow ====
// Board: Arduino Uno
// LEDs on D3 (through 330 Ω)

#include <FastLED.h>
#include <math.h>
#include <Wire.h>
#include <rgb_lcd.h>
#include <Adafruit_VS1053.h>
extern Adafruit_VS1053_FilePlayer musicPlayer; // audio player (defined in whitenoise.ino)

// --- Pins and hardware ---
#define LED_PIN      3
#define NUM_LEDS     40 // number of leds being powered
#define LED_TYPE     WS2812B
#define COLOR_ORDER  GRB
#define USB_MA_LIMIT 500
#define mosfet_fan   5
#define mosfet_heat  10
#define CHRISTMAS_BUTTON_PIN 4  // special christmas mode :)

// Christmas mode
bool christmasMode    = false;
bool lastButtonState  = HIGH;   // for edge detection
bool lcdChristmasMode = false; // special christmas display ("It's christmas time")

// sensor and LCD global values
const float ideal_temp = 24.0; // ideal temperature of the hospital room
float g_tempC      = 0.0;  // latest temperature reading (in celsius)
int   g_lightLevel = 1;    // light level from 1-4 (1 is dimmest, 4 is brightest)
int   g_soundRaw   = 0;    // latest raw sound reading (0-1023)
int   avgSound     = 0;    // smoothed sound reading (0-1023) - after noise handling is implemented
bool  tooLoudMode  = false; // is a loud noise detected? triggers loud mode

// LCD timing
unsigned long lastLcdChange = 0;

// LCD flashing state for Xmas mode
unsigned long lastLcdFlash = 0;
bool lcdFlashState = false;  // false = red, true = green (alternates)

// LED strip + LCD objects
CRGB leds[NUM_LEDS];
rgb_lcd lcd; // I2C pins used by the LCD

// Thermistor (temperature sensor) constants
const int   B  = 4275;
const int   R0 = 100000;

// Sensors
int temp_sensor = A1;
int led_sensor  = A3;

// Threshold intervals for light
const int T_LOW   = 300; // too dim
const int T_MED   = 500; // good
const int T_HIGH  = 700; // too bright

// Prevents rapid changes (hysteresis)
const int HYST = 20;

// Simple smoothing state
uint16_t smoothValue = 0;

// Declaration of functions 
void setupWhiteNoise();
void updateWhiteNoise(unsigned long now);
void setupLCD();
void updateLCD(unsigned long now);
void handleClimate(float R, float temp);
void handleLeds(int value);
void christmasLed(uint16_t swapSpeedMs, uint8_t breathBPM, uint8_t minV, uint8_t maxV, uint8_t globalBright);
void startChristmasSong();

// LCD Xmas mode
void showChristmasLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("It's Christmas");
  lcd.setCursor(0, 1);
  lcd.print("time!");

  // Start flashing cycle
  lastLcdFlash = millis();
  lcdFlashState = false;  // start with red
  lcd.setRGB(255, 0, 0);  // red backlight
  lcdChristmasMode = true;
}

void clearChristmasLCD() {
  lcdChristmasMode = false;
  // Reset backlight to white
  lcd.setRGB(255, 255, 255);
  lcd.clear();
}

void updateChristmasLCDFlash() {
  if (!lcdChristmasMode) return;

  unsigned long now = millis();

  // Flash every 500 ms
  if (now - lastLcdFlash > 500) {
    lastLcdFlash = now;
    lcdFlashState = !lcdFlashState;

    if (lcdFlashState) {
      // Green
      lcd.setRGB(0, 255, 0);
    } else {
      // Red
      lcd.setRGB(255, 0, 0);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.setTimeout(50);

  // LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, USB_MA_LIMIT);
  FastLED.setBrightness(100);
  FastLED.clear(true);

  // Sensors, fan, heater
  pinMode(led_sensor, INPUT);    // light sensor
  pinMode(temp_sensor, INPUT);   // temp sensor
  pinMode(mosfet_fan, OUTPUT);   // fan
  pinMode(mosfet_heat, OUTPUT);  // heating pad

  // christmas mode pin
  pinMode(CHRISTMAS_BUTTON_PIN, INPUT_PULLUP);
  lastButtonState = digitalRead(CHRISTMAS_BUTTON_PIN);

  // Sound and lcd setup
  setupWhiteNoise(); // VS1053 + SD + sound sensor
  setupLCD();        // LCD display

  // Set a default LCD backlight color (white)
  lcd.setRGB(255, 255, 255);
}

void loop() {
  // timing
  unsigned long now = millis();

  // Christmas button state
  bool buttonState = digitalRead(CHRISTMAS_BUTTON_PIN); // LOW when pressed

  // Checks if button is activated (toggle on and off)
  if (lastButtonState == HIGH && buttonState == LOW) {
    Serial.println(F("Christmas button pressed!"));

    if (christmasMode || musicPlayer.playingMusic) {
      Serial.println(F("Stopping Christmas mode via button."));
      musicPlayer.stopPlaying();
      christmasMode = false;
      FastLED.clear(true);
      clearChristmasLCD();
    } else {
      // Nothing playing: start Xmas
      startChristmasSong();   // defined in WhiteNoise.ino
      if (musicPlayer.playingMusic) {
        christmasMode = true;
        Serial.println(F("Entering Christmas mode."));
        showChristmasLCD();
      } else {
        Serial.println(F("Christmas song failed to start (check SD filenames)."));
      }
    }
  }
  lastButtonState = buttonState;

  // christmas music + red/green flashing lights
  if (christmasMode) {
    // Alternating red/green with breathing
    christmasLed(500, 5, 40, 150, 255);

    // LCD: flashing red/green background
    updateChristmasLCDFlash();

    // When the song ends, VS1053 stops playing
    if (!musicPlayer.playingMusic) {
      Serial.println(F("Christmas song finished, exiting Xmas mode."));
      christmasMode = false;
      FastLED.clear(true);
      clearChristmasLCD();
    }

    // Clap-trigger white noise is disabled while music is playing, prevents looping
    updateWhiteNoise(now);  
    return;
  }

  // Sensor readings from light and temp
  int raw = analogRead(led_sensor);  // 0..1023
  int a   = analogRead(temp_sensor);

  // Light smoothing
  if (smoothValue == 0) {
    smoothValue = raw;
  }
  smoothValue = (smoothValue * 7 + raw) / 8;
  int value = smoothValue;

  // Thermistor math
  float R = 1023.0 / ((float)a) - 1.0;
  R = R0 * R;
  float temp = 1.0 / (log(R / R0) / B + 1 / 298.15) - 273.15;

  handleClimate(R, temp);   // fan + heater logic
  handleLeds(value);        // soothing glow based on intensity of light in the room

  // Store temp into global
  g_tempC = temp;

  // light sensor: level 1 to 4
  if (value <= T_LOW) {
    g_lightLevel = 1; // dimmest
  } else if (value <= T_MED) {
    g_lightLevel = 2;
  } else if (value <= T_HIGH) {
    g_lightLevel = 3;
  } else {
    g_lightLevel = 4; // brightness
  }

  // Stops detecting when christmas mode is on
  if (!lcdChristmasMode) {
    updateLCD(now);
  }

  // Clap-trigger white noise (no effect if music is already playing)
  updateWhiteNoise(now);
}
