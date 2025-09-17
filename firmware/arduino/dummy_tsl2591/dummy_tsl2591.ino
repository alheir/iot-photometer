// Dummy TSL2591 generator for ESP8266
// Emits: "ILLUMINANCE=<value>" terminated by CR (0x0D) every 2 seconds.

#define BAUDRATE        115200
#define SEND_INTERVALMS 50UL    // 50ms
#define WAVE_PERIOD_MS  10000UL   // 10 seconds for a full sine wave
#define LUX_MIN         0.5f
#define LUX_MAX         1200.0f
#define NOISE_AMPL      5.0f      // +/- noise in lux

unsigned long lastSend = 0;

void setup() {
  Serial.begin(BAUDRATE);
  delay(500);
  // Seed RNG
  randomSeed(analogRead(A0) + micros());

  Serial.println(F("ESP8266 dummy TSL2591 ready"));
}

static inline void sendIlluminance(float lux) {
  // Print without LF; only CR to match MATLAB/Python reader
  Serial.print(F("ILLUMINANCE="));
  Serial.print(lux, 6);
  Serial.print('\r');
}

void loop() {
  unsigned long now = millis();
  if (now - lastSend >= SEND_INTERVALMS) {
    lastSend = now;

    // Sine wave between LUX_MIN and LUX_MAX
    float phase = (2.0f * 3.14159265f) * (float)(now % WAVE_PERIOD_MS) / (float)WAVE_PERIOD_MS;
    float mid = 0.5f * (LUX_MIN + LUX_MAX);
    float amp = 0.5f * (LUX_MAX - LUX_MIN);
    float base = mid + amp * sinf(phase);

    // Add small uniform noise
    float noise = ((float)random(-1000, 1001) / 1000.0f) * NOISE_AMPL;

    float lux = base + noise;
    if (lux < 0.0f) lux = 0.0f;

    sendIlluminance(lux);
  }
}
