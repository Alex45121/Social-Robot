#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include "KT403A_Player.h"
#include "HUSKYLENS.h"
#include <Servo.h> 

#define LED_PIN       4
#define NUMPIXELS    74
#define TOUCH_PIN     A0
#define SERVO_PIN_1   6
#define SERVO_PIN_2   7

SoftwareSerial mp3(2, 3);   // MP3 module
KT403A<SoftwareSerial> Mp3Player;

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN);

int LED_BRIGHTNESS = 8;

HUSKYLENS huskylens;
HUSKYLENSResult face;
bool face_detected = false;
bool prev_touch_value = 0;

enum Emotion {NEUTRAL, SUPRISED, HAPPY, ANGRY, SAD};
Emotion emotion = NEUTRAL;

Servo servo1, servo2;
float servo1_pos = 20, servo2_pos = 90;
float servo1_target = 20, servo2_target = 90;
float servo1_speed = 0, servo2_speed = 0;
float smoothX = 160, smoothY = 120;

long timer1, timer2, timer3;

bool pc_connected = false;
float servo1_target_pc = 20, servo2_target_pc = 90;

// ---------------- EYE PATTERNS ----------------
byte neutral[] = {B0000,B01110,B011110,B0111110,B011110,B01110,B0000};
byte blink1[] = {B0000,B00000,B011110,B0111110,B011110,B00000,B0000};
byte blink2[] = {B0000,B00000,B000000,B1111111,B000000,B00000,B0000};
byte suprised[] = {B1111,B11111,B111111,B1111111,B111111,B11111,B1111};
byte happy[] = {B1111,B11111,B111111,B1100011,B000000,B00000,B0000};
byte angry[] = {B0000,B10000,B110000,B1111000,B111110,B11111,B1111};
byte sad[] = {B0000,B00001,B000011,B0001111,B011111,B11111,B1111};

// ---------------- SETUP ----------------
void setup() {
  pinMode(TOUCH_PIN, INPUT);

  pixels.begin();
  Serial.begin(115200);

  // ✅ FIXED MP3 PART
  mp3.begin(9600);
  delay(1000);
  Mp3Player.init(mp3);
  Mp3Player.volume(30);

  // HuskyLens
  Wire.begin();
  while (!huskylens.begin(Wire)) {
    Serial.println(F("HuskyLens failed!"));
    delay(100);
  }

  servo1.attach(SERVO_PIN_1);
  servo2.attach(SERVO_PIN_2);
  servo1.write(20);
  servo2.write(90);
  
}

// ---------------- LOOP ----------------
void loop() {
  if (millis() - timer1 >= 20){
    timer1 = millis();
    move_servos();
    husky_lens();
    touch_sensor();
    run_emotions();
  }

  if (millis() - timer2 >= 10){
    timer2 = millis();
    communication();
  }

  //if (millis() - timer3 >= 100) {
  //timer3 = millis();
  //Serial.println(face_detected ? "FACE:1" : "FACE:0");
  //}
}

// ---------------- TOUCH ----------------
void touch_sensor() {
  bool touch_value = digitalRead(TOUCH_PIN);

  if (touch_value && !prev_touch_value) {
    Mp3Player.next();   // ✅ FIXED

    switch (emotion) {
      case NEUTRAL: emotion = SUPRISED; break;
      case SUPRISED: emotion = HAPPY; break;
      case HAPPY: emotion = ANGRY; break;
      case ANGRY: emotion = SAD; break;
      case SAD: emotion = NEUTRAL; break;
    }
  }
  prev_touch_value = touch_value;
}

// ---------------- EMOTIONS ----------------
void run_emotions(){
  pixels.clear();
  Serial.print("EMOTION=");
  Serial.print(emotion);
  Serial.print(" pc_connected=");
  Serial.println(pc_connected);  

  switch (emotion) {
    case NEUTRAL:
      if (millis() % 5000 < 150) display_eyes(blink1, 125);
      else if (millis() % 5000 < 300) display_eyes(blink2, 125);
      else if (millis() % 5000 < 450) display_eyes(blink1, 125);
      else display_eyes(neutral, 125);

      if (face_detected) {
        smoothX = smoothX * 0.6 + face.xCenter * 0.35;
        smoothY = smoothY * 0.75 + face.yCenter * 0.3;

        float offsetX = smoothX - 160;
        float offsetY = smoothY - 120;

        if (abs(offsetX) > 60) {
          servo2_target = 90 + offsetX / 320.0 * -40.0;
          servo2_target = constrain(servo2_target, 20, 180);
        }
        if (abs(offsetY) > 40) {
          servo1_target = 20 + offsetY / 240.0 * 50.0;
          servo1_target = constrain(servo1_target, 0, 90);
        }
       
        Serial.print(" faceY=");   Serial.print(face.yCenter);
        Serial.print(" | servo1_target="); Serial.print(servo1_target);
        Serial.print(" servo2_target="); Serial.println(servo2_target);
      }
      break;

    case HAPPY:
      display_eyes(happy, 80);
      servo1_target = 20 + 8.0 * sin(millis() / 500.0);   // tilt around 20
      servo2_target = 90 + 15.0 * cos(millis() / 400.0);  // pan around 90
      break;

    case SAD:
      display_eyes(sad, 150);
      servo1_target = 35 + 10.0 * cos(millis() / 500.0);  // look slightly down (sad)
      servo2_target = 90 + 3.0 * sin(millis() / 400.0);
      break;

    case ANGRY:
      display_eyes(angry, 0);
      servo1_target = 20 + 8.0 * sin(millis() / 250.0);   // quick shakes
      servo2_target = 90 + 12.0 * cos(millis() / 175.0);
      break;

    case SUPRISED:
      display_eyes(suprised, 125);
      servo1_target = 10;   // jolt up (surprised looks up)
      servo2_target = 90;
      break;
  }

  pixels.show();
}

// ---------------- EYES ----------------
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

// ---------------- HUSKY ----------------
void husky_lens() {
  if (!huskylens.request()) {}
  else if (!huskylens.available()) {
    face_detected = false;
  } else {
    face_detected = false;
    int face_index = 0;
    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();
      if (result.command == COMMAND_RETURN_BLOCK) {
        if (face_index == 0 || result.ID == 1) face = result;
        face_index++;
        face_detected = true;
      }
    }
  }
}

// ---------------- SERVOS ----------------
void move_servos(){
  float t1 = (pc_connected && emotion != NEUTRAL) ? servo1_target_pc : servo1_target;
  float t2 = (pc_connected && emotion != NEUTRAL) ? servo2_target_pc : servo2_target;

  servo1_pos += constrain(t1 - servo1_pos, -2, 2);
  servo2_pos += constrain(t2 - servo2_pos, -2, 2);

  servo1.write(servo1_pos);
  servo2.write(servo2_pos);
}

// ---------------- COMM ----------------
void communication() {
  char val = ' ';
  String data = "";
  if (Serial.available()) {
    do {
      val = Serial.read();
      if (val != -1) data = data + val;
    }
    while ( val != -1);
  }

  // data is a string of what we received, we will split it into the different values
  // We receive multiple values from our PC as in "123,abc,123,"
  // We can then split this string and extract the values out.
  if (data.length() > 1 && data.charAt(data.length() - 1) == ',') {
    Serial.print(data);
    pc_connected = true; // Once we get a message from the PC, we turn off the touch sensor and do everything with input from the PC

    String value;
    for (int i = 0; data.length() > 0; i++){
      value = data.substring(0, data.indexOf(','));
      data = data.substring(data.indexOf(',') + 1, data.length());

      if (i == 0) servo1_target_pc = value.toInt();
      if (i == 1) servo2_target_pc = value.toInt();
      if (i == 2) {
        if (value == "NEUTRAL") emotion = NEUTRAL;
        if (value == "SUPRISED") emotion = SUPRISED;
        if (value == "HAPPY") emotion = HAPPY;
        if (value == "ANGRY") emotion = ANGRY;
        if (value == "SAD") emotion = SAD;
      }
      // If more values are needed, add other lines here, e.g. if (i == 3) ...
    }
  }
}