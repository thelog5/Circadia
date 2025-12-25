// whitenoise.ino
// Temp sensor math + fan/heater control

#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>

// VS1053 pins
#define VS1053_RESET  8     // Reset line
#define VS1053_CS     6     // MP3 chip select
#define VS1053_DCS    7     // MP3 data/command select
#define CARDCS        9     // SD card chip select
#define DREQ          2     // Data request, must be interrupt-capable

#define SOUND_SENSOR_PIN A2

// Threshold
int soundThreshold = 800;

// State
bool wasLoud = false;

// Cooldown after each play
unsigned long lastTriggerTime = 0;
const unsigned long cooldownMs = 2000;   // 2 seconds
unsigned long lastPrint = 0; // debugging, prints to serial monitor

// Shared player object
Adafruit_VS1053_FilePlayer musicPlayer(
  VS1053_RESET, VS1053_CS, VS1053_DCS, DREQ, CARDCS
);

// g_soundRaw is defined in Delirium.ino
extern int g_soundRaw;

int soundToDb(int raw) {
  if (raw < 10) raw = 10;
  long scaled = (long)raw * 80L;
  return (int)(scaled / 1023L);
}

void setupWhiteNoise() {
  Serial.println(F("Init VS1053 + SD..."));

  pinMode(SOUND_SENSOR_PIN, INPUT);

  // Initialize VS1053
  if (!musicPlayer.begin()) {
    Serial.println(F("Couldn't find VS1053 chip"));
    while (1);
  }
  Serial.println(F("VS1053 found"));

  // Initialize SD card
  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD card failed or not present"));
    while (1);
  }
  Serial.println(F("SD card OK"));

  // Volume: 0 = loudest, 255 = mute
  musicPlayer.setVolume(0, 0);

  // Feed via interrupt
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);

  Serial.println(F("White noise + Xmas ready. Clap -> white.mp3, button -> christmas.mp3"));
}

// Button-triggered Christmas
void startChristmasSong() {
  if (musicPlayer.playingMusic) {
    Serial.println(F("startChristmasSong() called, but music already playing."));
    return;
  }

  Serial.println(F("Starting christmas.mp3"));
  if (!musicPlayer.startPlayingFile("xmas.mp3")) {
    Serial.println(F("Could not open christmas.mp3"));
  } else {
    Serial.println(F("Christmas track playing..."));
  }
}

// Sound-triggered white noise
void updateWhiteNoise(unsigned long now) {
  // If music is playing, ignore sensor completely
  if (musicPlayer.playingMusic) {
    return;
  }

  // Simple cooldown after last trigger
  if (now - lastTriggerTime < cooldownMs) {
    return;
  }

  // Read sound level
  // Read and filter sound
  int raw = analogRead(SOUND_SENSOR_PIN);
  g_soundRaw = raw; 

  // Smooth with exponential moving average (EMA) filter (noise handling)
  static int filtered = 0;
  if (filtered == 0) filtered = raw;
  filtered = (filtered * 3 + raw) / 4;  // 3/4 filter

  int soundValue = filtered;  // updates the raw value with the filtered value

  g_soundRaw = soundValue;

  if (now - lastPrint > 300) {
    lastPrint = now;
    Serial.print(F("Sound level: "));
    Serial.println(soundValue);
  }

  bool isLoud = (g_soundRaw > soundThreshold);
  // Check if it's loud
  if (isLoud) {
    Serial.println("here");
    tooLoudMode = true;
    lastTriggerTime = now;
  } else if (now - lastTriggerTime > 1500) {
    // 1.5 seconds with no loud noise, resets
    tooLoudMode = false;
  }

  // Rising edge: went from not loud to loud
  if (isLoud && !wasLoud) {
    Serial.println(F("Loud sound detected!"));
    Serial.println(F("Starting white.mp3"));

    if (!musicPlayer.startPlayingFile("white.mp3")) {
      Serial.println(F("Could not open white.mp3"));
    } else {
      Serial.println(F("White noise playing..."));
      lastTriggerTime = now;   // start cooldown from this moment
    }
  }

  wasLoud = isLoud;
}
