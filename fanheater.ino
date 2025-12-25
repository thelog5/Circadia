// fanheater.ino
// Temp sensor math + fan/heater control

void handleClimate(float R, float temp) {
  // ideal temperature of a hospital room (24 degrees)
  // Below is an exponential function that controls how fast the temperature is changing
  // Prevents the patient from experiencing extreme temperature changes by easing them in slowly
  
  // Fan min = 190, Heater min = 100 (min means its turned off at this value)
  static float fanPWM  = 0.0;
  static float heatPWM = 0.0;

  // Target room temperature and "comfort band"
  const float maxDelta   = 6.0;   // if temp diff is greater than 6, we cap at 255 to prevent rapid changes in temperature
  const float FAN_MIN_PWM   = 190.0;  // minimum fan speed
  const float HEAT_MIN_PWM  = 100.0;  // minimum heater power
  
  float fanTarget  = 0.0;
  float heatTarget = 0.0;

  // Too warm, turn on fan
  if (temp > ideal_temp) {
    float diff = temp - (ideal_temp);
    if (diff > maxDelta) diff = maxDelta;

    // Convert the temperature difference into a fraction 
    // 0  → exactly ideal_temp
    // 1  → maxDelta or more above ideal_temp
    float frac = diff / maxDelta;

    if (frac <= 0.0f) {
      // Fan off
      fanTarget = 0.0;
    } else {
      // Map frac (0 to 1) to a PWM range of FAN_MIN_PWM 255
      // So a small difference starts the fan gently,
      // and a large difference runs it close to max speed.
      fanTarget = FAN_MIN_PWM + frac * (255.0 - FAN_MIN_PWM);
    }

    // Heater off
    heatTarget = 0.0;
  }

  // Too cold, turn on heater
  else if (temp < ideal_temp) {
    float diff = (ideal_temp) - temp;
    if (diff > maxDelta) diff = maxDelta;
    // fractioned like above
    float frac = diff / maxDelta;

    if (frac <= 0.0f) {
      // Within the comfort band, heater off
      heatTarget = 0.0;    
    } else {
      // Map frac (0 to 1) into HEAT_MIN_PWM 255
      // Slightly cold, gentle heating
      // Very cold (>= maxDelta), near full power
      heatTarget = HEAT_MIN_PWM + frac * (255.0 - HEAT_MIN_PWM);
    }

    fanTarget = 0.0;
  }

  else {
    // reached the ideal temp
    fanTarget  = 0.0;
    heatTarget = 0.0;
  }

  // Smoothing
  const float alpha = 0.2;

  // Exponential smoothing: don't jump directly to fanTarget/heatTarget.
  // Instead, move a fraction (alpha) of the difference each loop.
  // This creates a gradual ramp in speed/power rather than sudden changes.
  fanPWM  += alpha * (fanTarget  - fanPWM);
  heatPWM += alpha * (heatTarget - heatPWM);

  if (fanPWM   < 0)   fanPWM  = 0;
  if (fanPWM   > 255) fanPWM  = 255;
  if (heatPWM  < 0)   heatPWM = 0;
  if (heatPWM  > 255) heatPWM = 255;
  
  // remember 255 is max, 0 is min REMEMBER 
  analogWrite(mosfet_fan,  (int) fanPWM);
  analogWrite(mosfet_heat, (int) heatPWM);
}
