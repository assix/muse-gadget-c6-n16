/*
 * ESP32-C6 onboard RGB LED test (WS2812 / NeoPixel)
 *
 * Verifies which GPIO your board's addressable RGB LED is on.
 * Default guess: GPIO 8 (common on ESP32-C6 devkits).
 *
 * Setup:
 *  1. Arduino IDE -> Library Manager -> install "Adafruit NeoPixel"
 *  2. Tools -> Board -> "ESP32C6 Dev Module" (needs ESP32 Arduino core 3.x)
 *  3. Tools -> "USB CDC On Boot" -> Enabled  (needed for Serial on C6 native USB)
 *  4. Upload, then open Serial Monitor at 115200 baud.
 */

#include <Adafruit_NeoPixel.h>

#define LED_PIN    8   // <-- change this if your RGB LED is on a different GPIO
#define LED_COUNT  1   // most devkits have a single onboard RGB LED

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void showColor(const char *name, uint8_t r, uint8_t g, uint8_t b) {
  Serial.print("Showing: ");
  Serial.println(name);
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
  delay(1200);
}

void setup() {
  Serial.begin(115200);
  delay(1500);  // give the USB serial a moment to come up
  Serial.println();
  Serial.println("=== ESP32-C6 RGB LED test ===");
  Serial.print("GPIO under test: ");
  Serial.println(LED_PIN);

  strip.begin();
  strip.setBrightness(40);   // these LEDs are blinding at full power
  strip.show();              // all off
  delay(500);
}

void loop() {
  showColor("RED",   255, 0, 0);
  showColor("GREEN", 0, 255, 0);
  showColor("BLUE",  0, 0, 255);
  showColor("WHITE", 255, 255, 255);
  Serial.println("OFF");
  strip.clear();
  strip.show();
  delay(1200);
  Serial.println("--- cycle complete ---");
}
