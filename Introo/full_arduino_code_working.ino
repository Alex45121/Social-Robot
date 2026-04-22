#include <Adafruit_NeoPixel.h>

#define LED_PIN     4
#define NUMPIXELS   74

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN);

int LED_BRIGHTNESS = 8;

// ---------------- EMOTIONS ----------------
enum Emotion {NEUTRAL, SUPRISED, HAPPY, ANGRY, SAD};

// Loop control
bool loop_eyes = false;
int loop_index = 0;
long loop_timer = 0;

// ---------------- EYE PATTERNS ----------------
byte neutral[]  = {B0000,B01110,B011110,B0111110,B011110,B01110,B0000};
byte blink1[]   = {B0000,B00000,B011110,B0111110,B011110,B00000,B0000};
byte blink2[]   = {B0000,B00000,B000000,B1111111,B000000,B00000,B0000};
byte suprised[] = {B1111,B11111,B111111,B1111111,B111111,B11111,B1111};
byte happy[]    = {B1111,B11111,B111111,B1100011,B000000,B00000,B0000};
byte angry[]    = {B0000,B10000,B110000,B1111000,B111110,B11111,B1111};
byte sad[]      = {B0000,B00001,B000011,B0001111,B011111,B11111,B1111};

// ---------------- SETUP ----------------
void setup() {
  pixels.begin();
  Serial.begin(115200);
}

// ---------------- LOOP ----------------
void loop() {
  communication();
  run_eyes();
}

// ---------------- SERIAL COMM ----------------
void communication() {
  String data = "";

  while (Serial.available()) {
    char c = Serial.read();
    data += c;
  }

  if (data.endsWith(",")) {
    if (data.indexOf("LOOP") >= 0) {
      loop_eyes = true;
    }
    if (data.indexOf("STOP") >= 0) {
      loop_eyes = false;
    }
  }
}

// ---------------- EYE LOOP ----------------
void run_eyes() {
  pixels.clear();

  if (loop_eyes) {
    if (millis() - loop_timer > 800) {
      loop_timer = millis();
      loop_index = (loop_index + 1) % 5;
    }

    switch (loop_index) {
      case 0: display_eyes(neutral, 125); break;
      case 1: display_eyes(happy, 80); break;
      case 2: display_eyes(sad, 150); break;
      case 3: display_eyes(angry, 0); break;
      case 4: display_eyes(suprised, 125); break;
    }
  } else {
    // Default neutral blinking
    if (millis() % 5000 < 150) display_eyes(blink1, 125);
    else if (millis() % 5000 < 300) display_eyes(blink2, 125);
    else if (millis() % 5000 < 450) display_eyes(blink1, 125);
    else display_eyes(neutral, 125);
  }

  pixels.show();
}

// ---------------- EYE DRAWING ----------------
void display_eyes(byte arr[], int hue){
   display_eye(arr, hue, true);
   display_eye(arr, hue, false);
}

void display_eye(byte arr[], int hue, bool left) {
  int rows[] = {4,5,6,7,6,5,4};
  int index = (left) ? 0 : 37;

  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < rows[i]; j++) {
      int brightness = LED_BRIGHTNESS * bitRead(arr[i], (left) ? rows[i]-1-j : j);
      pixels.setPixelColor(index, pixels.ColorHSV(hue * 256, 255, brightness));
      index++;
    }
  }
}