// NeoTrellis M4 Simple LED Test
// Just verify Arduino CLI build and flash process works

#include <Adafruit_NeoTrellisM4.h>

Adafruit_NeoTrellisM4 trellis;

void setup() {
    Serial.begin(115200);
    Serial.println("NeoTrellis M4 Simple Test");
    
    trellis.begin();
    trellis.setBrightness(80);
    
    // Set first 8 LEDs to red
    for (int i = 0; i < 8; i++) {
        trellis.setPixelColor(i, 0xFF0000);
    }
    trellis.show();
    
    Serial.println("Red LEDs should be visible!");
}

void loop() {
    delay(1000);
    
    // Toggle between red and off every second
    static bool on = true;
    
    for (int i = 0; i < 8; i++) {
        if (on) {
            trellis.setPixelColor(i, 0x000000);  // Off
        } else {
            trellis.setPixelColor(i, 0xFF0000);  // Red
        }
    }
    trellis.show();
    on = !on;
    
    Serial.println(on ? "LEDs ON" : "LEDs OFF");
}