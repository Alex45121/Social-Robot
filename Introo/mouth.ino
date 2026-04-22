#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

#ifndef _BV
  #define _BV(bit) (1<<(bit))
#endif

Adafruit_8x8matrix matrix = Adafruit_8x8matrix();

// Your images (8x8 packed into 64-bit values)
const uint64_t IMAGES[] = {
    0x00007e0000000000,
  0x003c420000000000,
  0x003c4242423c0000,
  0x00005a2400000000,
  0x00007e0000000000,
  0x00245a0000000000
};

const int IMAGES_LEN = sizeof(IMAGES) / sizeof(IMAGES[0]);

int i = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("I2C Matrix Animation");

  matrix.begin(0x70);   // change if needed
  matrix.setBrightness(5);
  matrix.clear();
  matrix.writeDisplay();
}

// Convert uint64_t image to 8x8 matrix
void showImage(uint64_t img) {
  for (int y = 0; y < 8; y++) {
    uint8_t row = (img >> (8 * y)) & 0xFF;

    for (int x = 0; x < 8; x++) {
      // FIX: flip X direction
      bool on = row & (1 << (7 - x));
      matrix.drawPixel((x + 1) % 8, y, on);
    }
  }

  matrix.writeDisplay();
}

void loop() {
  showImage(IMAGES[i]);

  i++;
  if (i >= IMAGES_LEN) i = 0;

  delay(300);
}