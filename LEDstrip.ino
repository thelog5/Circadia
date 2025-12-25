// LedStrip.ino
// LED breathing + brightness bands based on light sensor

void soothingGlow(
  const CRGB& colorA, // primary colour 
  const CRGB& colorB, // secondary colour
  uint8_t breathBPM, // slow breath
  uint8_t minV, // min brightness
  uint8_t maxV, // max brightness
  uint8_t globalBright // global brightness
) {
  FastLED.setBrightness(globalBright);

  uint8_t breath = beatsin8(breathBPM, minV, maxV); // fluctuates between two similar lighting hues to micmic soothing breathing

  uint8_t blendAmount = beatsin8(2, 0, 255);
  CRGB currentColor   = blend(colorA, colorB, blendAmount);

  fill_solid(leds, NUM_LEDS, currentColor);
  nscale8_video(leds, NUM_LEDS, breath);

  FastLED.show();
}


void handleLeds(int value) {
  // When sound reaches a threshold for being too loud
  if (tooLoudMode) {
    // Soothing low-brightness blue breathing
    soothingGlow(
      CRGB(0, 40, 100), // medium blue
      CRGB(0, 90, 255),
      4,          
      30, 120,   
      60     
    );
    return;
  }

  if (value <= T_LOW - HYST) {
    // low light, bright 
    soothingGlow(
      CRGB(200, 60, 15), // bright warm red (little peachy)
      CRGB(180, 50, 10),
      4,
      30, 120,
      255
    );
  }
  else if (value <= T_MED + HYST) {
    soothingGlow(
      CRGB(255, 80, 25), // bright warm red-peachy
      CRGB(255, 100, 35),
      5,
      40, 150,
      200
    );
  }
  else if (value <= T_HIGH + HYST) {
    soothingGlow(
      CRGB(255, 90, 30), // medium warm peach
      CRGB(255, 115, 45),
      6,
      60, 180,
      30
    );
  }
  else {
    soothingGlow(
      CRGB(255, 110, 40), // dim warm peach
      CRGB(255, 130, 60),
      8,
      120, 120,
      20
    );
  }
}

void christmasLed(uint16_t swapSpeedMs, uint8_t breathBPM, uint8_t minV, uint8_t maxV, uint8_t globalBright) {
  FastLED.setBrightness(globalBright);

  // colour swap (slight delay before swapping), prevents rapid fluctuations
  static uint32_t lastSwap = 0;
  static bool swapState = false;

  if (millis() - lastSwap >= swapSpeedMs) {
    swapState = !swapState;   // swap pattern
    lastSwap = millis();
  }

  // breathing brightness
  uint8_t breath = beatsin8(breathBPM, minV, maxV);

  // Alternating pattern (green and red)
  for (int i = 0; i < NUM_LEDS; i++) {
    bool even = (i % 2 == 0);

    if (swapState) {
      // Pattern A
      leds[i] = even ? CRGB::Red : CRGB::Green;
    } else {
      // Pattern B (swapped)
      leds[i] = even ? CRGB::Green : CRGB::Red;
    }
  }

  // Apply breathing effect
  nscale8_video(leds, NUM_LEDS, breath);

  FastLED.show();
}
