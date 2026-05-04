#include <Adafruit_NeoPixel.h>

#define LED_PIN 4
#define NUMPIXELS 74

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN);

int LED_BRIGHTNESS = 8;

enum Emotion { NEUTRAL, SUPRISED, HAPPY, ANGRY, SAD };
Emotion emotion = NEUTRAL;

// ---------------- BLINK STATE ----------------
bool do_blink = false;
int blink_step = 0;
unsigned long blink_timer = 0;

// ---------------- EYE PATTERNS ----------------
byte neutral[]   = { B0000, B01110, B011110, B0111110, B011110, B01110, B0000 };
byte blink1[]    = { B0000, B00000, B011110, B0111110, B011110, B00000, B0000 };
byte blink2[]    = { B0000, B00000, B000000, B1111111, B000000, B00000, B0000 };
byte suprised[]  = { B1111, B11111, B111111, B1111111, B111111, B11111, B1111 };
byte happy[]     = { B1111, B11111, B111111, B1100011, B000000, B00000, B0000 };
byte angry[]     = { B0000, B10000, B110000, B1111000, B111110, B11111, B1111 };
byte sad[]       = { B0000, B00001, B000011, B0001111, B011111, B11111, B1111 };

// ---------------- FUNCTION PROTOTYPES ----------------
void run_eyes();
void communication();
void trigger_blink();
void display_eyes(byte arr[], int hue);
void display_eye(byte arr[], int hue, bool left);

// ---------------- SETUP ----------------
void setup() {
  pixels.begin();
  Serial.begin(115200);
  Serial.println("Ready");
}

// ---------------- LOOP ----------------
void loop() {
  communication();
  run_eyes();
}

// ---------------- SERIAL ----------------
void communication() {
  if (!Serial.available()) return;

  String data = Serial.readStringUntil('\n');
  data.trim();

  if (data.length() == 0) return;

  Serial.print("Received: ");
  Serial.println(data);

  if (data == "B") {
    trigger_blink();
    return;
  }

  if (data == "HAPPY") emotion = HAPPY;
  else if (data == "SAD") emotion = SAD;
  else if (data == "ANGRY") emotion = ANGRY;
  else if (data == "NEUTRAL") emotion = NEUTRAL;
  else if (data == "SUPRISED") emotion = SUPRISED;
}

// ---------------- BLINK ----------------
void trigger_blink() {
  do_blink = true;
  blink_step = 0;
  blink_timer = millis();
}

// ---------------- EYES ----------------
void run_eyes() {
  pixels.clear();

  if (do_blink) {

    if (millis() - blink_timer > 120) {
      blink_timer = millis();
      blink_step++;
    }

    switch (blink_step) {
      case 0: display_eyes(neutral, 125); break;
      case 1: display_eyes(blink1, 125); break;
      case 2: display_eyes(blink2, 125); break;
      case 3: display_eyes(blink1, 125); break;
      case 4:
        display_eyes(neutral, 125);
        do_blink = false;
        Serial.println("BLINK_DONE");
        break;
    }

  } else {

    switch (emotion) {
      case NEUTRAL:  display_eyes(neutral, 125); break;
      case HAPPY:    display_eyes(happy, 80); break;
      case SAD:      display_eyes(sad, 150); break;
      case ANGRY:    display_eyes(angry, 0); break;
      case SUPRISED: display_eyes(suprised, 125); break;
    }
  }

  pixels.show();
}

// ---------------- DRAW ----------------
void display_eyes(byte arr[], int hue) {
  display_eye(arr, hue, true);
  display_eye(arr, hue, false);
}

void display_eye(byte arr[], int hue, bool left) {
  int rows[] = {4, 5, 6, 7, 6, 5, 4};
  int index = left ? 0 : 37;

  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < rows[i]; j++) {

      int brightness = LED_BRIGHTNESS * bitRead(arr[i], left ? rows[i] - 1 - j : j);

      pixels.setPixelColor(
        index,
        pixels.ColorHSV(hue * 256, 255, brightness)
      );

      index++;
    }
  }
}