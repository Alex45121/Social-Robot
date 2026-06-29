// ============================================================================
//  GroveBot - Merged Robot Brain
//  Base: already-combined code
//  LED control merged from the updated LED-only code
//
//  Commands over Serial:
//    N = Neutral
//    Q = Questioning
//    C = Comfort
//    H = Happy
//    S = Sad
//    F = Frown
//    B = Breathe
//
//  Sends "FACE:1" / "FACE:0" to the PC every 100ms.
// ============================================================================

#include <Adafruit_NeoPixel.h>
#include "HUSKYLENS.h"
#include <Servo.h>
#include <Wire.h>
#include <ctype.h>

// ---------------- PIN SETUP ----------------
#define LEFT_LED_PIN    4      // left eye matrix
#define RIGHT_LED_PIN   5      // right eye matrix
#define PIXELS_PER_EYE  37

#define SERVO_PIN_1     7      // head pan  (external power, Arduino signal)
#define SERVO_PIN_2     8      // head tilt
#define LEFT_EAR_PIN    9      // ear servo 1
#define RIGHT_EAR_PIN   3      // ear servo 2
#define PURR_PIN        6      // third servo for purring / breathing motion

#define TOUCH_PIN       A0
// ---------------- EYE HARDWARE / TUNING ----------------
#define LEFT_ROTATION_STEPS   0
#define RIGHT_ROTATION_STEPS  0

#define LEFT_COMFORT_ROTATION_STEPS   0
#define RIGHT_COMFORT_ROTATION_STEPS  0

#define LEFT_HAPPY_ROTATION_STEPS     0
#define RIGHT_HAPPY_ROTATION_STEPS    0

#define LEFT_SAD_ROTATION_STEPS       0
#define RIGHT_SAD_ROTATION_STEPS      0

#define LEFT_FROWN_ROTATION_STEPS     0
#define RIGHT_FROWN_ROTATION_STEPS    0

#define LEFT_BLINK_ROTATION_STEPS     0
#define RIGHT_BLINK_ROTATION_STEPS    0

#define LEFT_BREATHE_ROTATION_STEPS   0
#define RIGHT_BREATHE_ROTATION_STEPS  0

#define RIGHT_REVERSED true
#define BLINK45_DIRECTION 1

#define BREATHING_CYCLE_MS 10000
#define BREATHING_START_HOLD_MS 700
#define BREATHING_MIN_BRIGHTNESS 1
#define BREATHING_MAX_BRIGHTNESS 18

// ---------------- EAR ANGLES PER EMOTION ----------------
const int LEFT_NEUTRAL  = 90,  RIGHT_NEUTRAL  = 90;
const int LEFT_QUESTION = 60,  RIGHT_QUESTION = 120;
const int LEFT_COMFORT  = 100, RIGHT_COMFORT  = 80;
const int LEFT_HAPPY    = 135, RIGHT_HAPPY    = 45;
const int LEFT_SAD      = 45,  RIGHT_SAD      = 135;
const int LEFT_FROWN    = 30,  RIGHT_FROWN    = 150;
const int LEFT_BREATHE_IN  = 150, RIGHT_BREATHE_IN  = 30;
const int LEFT_BREATHE_OUT = 90,  RIGHT_BREATHE_OUT = 90;

// ---------------- PURR SERVO SETTINGS ----------------
const int PURR_CENTER = 90;
const int PURR_RANGE = 10;              // small fast wiggle for happy / comfort
const int PURR_BREATHE_RANGE = 35;      // larger slow movement for breathing
const unsigned long PURR_STEP_MS = 40;
const unsigned long PURR_DURATION_MS = 4800;

// ---------------- BREATHING EAR / PURR TIMING ----------------
const unsigned long BREATHE_IN_MS = 4000;
const unsigned long BREATHE_HOLD_MS = 2000;
const unsigned long BREATHE_OUT_MS = 4000;

unsigned long emotionStartTime = 0;

const unsigned long EMOTION_DURATION = 5000;             // normal emotions = 5 sec
const unsigned long BREATHING_EMOTION_DURATION = 15000;  // breathing = 15 sec

// ---------------- OBJECTS ----------------
Adafruit_NeoPixel leftEye(PIXELS_PER_EYE, LEFT_LED_PIN);
Adafruit_NeoPixel rightEye(PIXELS_PER_EYE, RIGHT_LED_PIN);

HUSKYLENS huskylens;
HUSKYLENSResult face;

Servo servo1, servo2;       // head pan/tilt
Servo leftEar, rightEar;    // ears
Servo purrServo;            // third servo for purring / breathing motion

// ---------------- STATE ----------------
enum Emotion {
  NEUTRAL,
  QUESTIONING,
  COMFORT,
  HAPPY,
  SAD,
  FROWN,
  BREATHE
};

Emotion emotion = NEUTRAL;
Emotion lastEarEmotion = NEUTRAL;

bool face_detected = false;
bool prev_touch_value = 0;
unsigned long lastFaceSeen = 0;

const int SERVO1_NEUTRAL_POS = 20;
const int SERVO2_NEUTRAL_POS = 90;

const int FACE_DEADZONE_X = 50;
const int FACE_DEADZONE_Y = 45;

const float FACE_SMOOTHING = 0.12;
const float FACE_PAN_GAIN = 0.04;
const float FACE_TILT_GAIN = 0.025;

float servo1_pos = SERVO1_NEUTRAL_POS;
float servo2_pos = SERVO2_NEUTRAL_POS;
float servo1_target = SERVO1_NEUTRAL_POS;
float servo2_target = SERVO2_NEUTRAL_POS;
float anchor1 = SERVO1_NEUTRAL_POS, anchor2 = SERVO2_NEUTRAL_POS;

float smoothX = 160;
float smoothY = 120;

int LED_BRIGHTNESS = 6;
unsigned long last_blink_trigger = 0;
int blink_interval = 4500;
unsigned long breathing_start_time = 0;
bool do_blink = false;
unsigned long blink_timer = 0;

bool purr_active = false;
unsigned long purr_start_time = 0;
unsigned long purr_last_step = 0;
unsigned long purr_duration_ms = 0;
bool purr_toggle = false;
bool purrServoAttached = false;

long timer1, timer2, timer3, timer4;

// ---------------- HEX COORDINATE TYPE ----------------
typedef struct {
  int q;
  int r;
} HexCoord;

int rows[] = { 4, 5, 6, 7, 6, 5, 4 };

// ---------------- FUNCTION DECLARATIONS ----------------
void communication();
void touch_sensor();
void update_head();
void move_servos();
void update_ears();
void moveBothEars(int leftTarget, int rightTarget);
void update_purr_servo();
void attachPurrServo();
void detachPurrServo();
void startPurr(unsigned long durationMs);
void stopPurr();
void stopAllVibration();
void update_breathing_motion();
int lerpInt(int startValue, int endValue, float amount);
void husky_lens();

void run_eyes();
void trigger_blink();
int getEmotionHue(int e);
float getBreathingWave();
int getBreathingBrightness();

HexCoord indexToCoord(int index);
int coordToIndex(HexCoord c);
HexCoord rotateHex120(HexCoord c);
HexCoord rotateHex240(HexCoord c);
HexCoord applyHexRotation(HexCoord c, int steps);
int getHexRing(HexCoord c);

void display_eyes(byte arr[], int hue, int leftRotation, int rightRotation);
void display_eye(Adafruit_NeoPixel &eye, byte arr[], int hue, bool isLeftEye, int rotationSteps);

void display_breathing_eyes(int hue, int leftRotation, int rightRotation);
void display_breathing_eye(Adafruit_NeoPixel &eye, int hue, bool isLeftEye, int rotationSteps);

void display_blink45(int hue, int blinkStage);
void display_blink45_eye(Adafruit_NeoPixel &eye, int hue, int blinkStage, bool isLeftEye, int rotationSteps);

void setMappedPixel(Adafruit_NeoPixel &eye, int index, int hue, int brightness, bool isLeftEye, int rotationSteps);

// ---------------- EYE PATTERNS ----------------
byte neutral[]     = { B0000, B01110, B011110, B0111110, B011110, B01110, B0000 };
byte questioning[] = { B1111, B11111, B111111, B1111111, B111111, B11111, B1111 };
byte comfort[]     = { B0000, B00000, B100001, B1100011, B111111, B11111, B1111 };
byte happy[]       = { B1111, B11000, B110000, B1100000, B100000, B10000, B1000 };
byte sad[]         = { B0000, B00001, B000011, B0001111, B011111, B11111, B1111 };
byte frown[]       = { B1000, B11000, B111000, B1110000, B110000, B11000, B1100 };

// ============================================================================
//  SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);

  // Eyes
  leftEye.begin();
  rightEye.begin();

  leftEye.clear();
  rightEye.clear();

  leftEye.show();
  rightEye.show();

  breathing_start_time = millis();
  randomSeed(analogRead(A2));

  // Head servos
  servo1.attach(SERVO_PIN_1);
  servo2.attach(SERVO_PIN_2);
  servo1.write(SERVO1_NEUTRAL_POS);
  servo2.write(SERVO2_NEUTRAL_POS);

  // Ears + purr servo
  leftEar.attach(LEFT_EAR_PIN);
  rightEar.attach(RIGHT_EAR_PIN);
  purrServo.attach(PURR_PIN);
  purrServoAttached = true;

  leftEar.write(LEFT_NEUTRAL);
  rightEar.write(RIGHT_NEUTRAL);
  purrServo.write(PURR_CENTER);
  delay(200);          // let it reach center once at startup
  purrServo.detach();  // no servo signal in neutral, prevents twitching
  purrServoAttached = false;

 

  // Touch
  pinMode(TOUCH_PIN, INPUT);

  // Camera
  Wire.begin();
  while (!huskylens.begin(Wire)) {
    Serial.println(F("HuskyLens failed!"));
    delay(100);
  }

  Serial.println("GroveBot ready");
}

// ============================================================================
//  MAIN LOOP
// ============================================================================
void loop() {
  // Auto-return to neutral after an emotion has shown for a while.
  // Breathing gets a longer duration than the other emotions.
  if (emotion != NEUTRAL) {
    unsigned long duration = (emotion == BREATHE)
                             ? BREATHING_EMOTION_DURATION
                             : EMOTION_DURATION;

    if (millis() - emotionStartTime > duration) {
      emotion = NEUTRAL;

      // Put head servos and purr servo back to neutral/home position
      servo1_target = SERVO1_NEUTRAL_POS;
      servo2_target = SERVO2_NEUTRAL_POS;
      stopAllVibration();
    }
  }

  // Head + tracking + ears 
  if (millis() - timer1 >= 40) {
    timer1 = millis();

    husky_lens();
    touch_sensor();

    update_head();
    move_servos();

    update_ears();
    update_breathing_motion();
    update_purr_servo();
  }

  // Eyes
  if (millis() - timer4 >= 30) {
    timer4 = millis();
    run_eyes();
  }

  // Serial input
  if (millis() - timer2 >= 10) {
    timer2 = millis();
    communication();
  }

  // Face status out to PC
  if (millis() - timer3 >= 100) {
    timer3 = millis();
  }
}

// ============================================================================
//  SERIAL COMMUNICATION
// ============================================================================
void communication() {
  if (Serial.available()) {
    char input = Serial.read();
    input = toupper(input);

    switch (input) {
      case 'N':
  emotion = NEUTRAL;
  emotionStartTime = millis();

  servo1_target = SERVO1_NEUTRAL_POS;
  servo2_target = SERVO2_NEUTRAL_POS;

  stopAllVibration();
  break;

case 'Q':
  emotion = QUESTIONING;
  emotionStartTime = millis();

  stopAllVibration();
  break;

case 'S':
  emotion = SAD;
  emotionStartTime = millis();

  stopAllVibration();
  break;

case 'F':
  emotion = FROWN;
  emotionStartTime = millis();

  stopAllVibration();
  break;

      case 'C':
        emotion = COMFORT;
        emotionStartTime = millis();
        break;

      case 'H':
        emotion = HAPPY;
        emotionStartTime = millis();
        break;

      case 'B':
        emotion = BREATHE;
        emotionStartTime = millis();

        // Start breathing from the middle at minimum brightness
        breathing_start_time = millis();
        LED_BRIGHTNESS = BREATHING_MIN_BRIGHTNESS;
        do_blink = false;
        stopAllVibration();
        attachPurrServo();   // breathing may use the purr servo slowly

        // Keep head calm during breathing
        servo1_target = SERVO1_NEUTRAL_POS;
        servo2_target = SERVO2_NEUTRAL_POS;
        break;

      case '\n':
      case '\r':
      case ' ':
        break;

      default:
        break;
    }
  }
}

// ============================================================================
//  TOUCH
// ============================================================================
void touch_sensor() {
  // Touch is disabled for now, because it was causing unwanted emotion switching.
  // If you want it back later, we can re-enable it with a cooldown.
}

// ============================================================================
//  HEAD
// ============================================================================
void update_head() {
  if (emotion == NEUTRAL) {
    // Face tracking (your working relative-nudge version)
    if (face_detected) {
      smoothX += (face.xCenter - smoothX) * FACE_SMOOTHING;
      smoothY += (face.yCenter - smoothY) * FACE_SMOOTHING;

      float offsetX = smoothX - 160;
      float offsetY = smoothY - 120;

      if (abs(offsetX) < FACE_DEADZONE_X) offsetX = 0;
      if (abs(offsetY) < FACE_DEADZONE_Y) offsetY = 0;

      servo2_target = SERVO2_NEUTRAL_POS + offsetX * -0.15;
      servo1_target = SERVO1_NEUTRAL_POS + offsetY *  0.15;

      servo2_target = constrain(servo2_target, 20, 160);
      servo1_target = constrain(servo1_target, 0, 90);
    } else {
      // No face → return to neutral center
      servo1_target = SERVO1_NEUTRAL_POS;
      servo2_target = SERVO2_NEUTRAL_POS;
    }
  } else {
    // ... their emotion gestures stay exactly the same ...
    // Emotion head gestures.
    switch (emotion) {
      case HAPPY:
        servo1_target = anchor1 - 5 + 4.0 * sin(millis() / 250.0);   // small nods near anchor
        servo2_target = anchor2 + 18.0 * sin(millis() / 200.0);       // quick happy shake
        break;
      case SAD:
        servo1_target = anchor1 + 8.0 + 6.0 * cos(millis() / 500.0);  // droop down from anchor
        servo2_target = anchor2 + 3.0 * sin(millis() / 400.0);
        break;
      case COMFORT:
        servo1_target = anchor1 + 4.0 * sin(millis() / 700.0);
        servo2_target = anchor2 + 5.0 * cos(millis() / 700.0);
        break;
      case QUESTIONING:
        servo1_target = anchor1 - 5.0 + 4.0 * sin(millis() / 500.0);
        servo2_target = anchor2 + 12.0 * cos(millis() / 400.0);
        break;
      case FROWN:
        servo1_target = anchor1 + 12.0;   // look down from anchor
        servo2_target = anchor2;
        break;
      case BREATHE:
        servo1_target = SERVO1_NEUTRAL_POS;  // breathing stays calm/centered
        servo2_target = SERVO2_NEUTRAL_POS;
        break;
    }
// Safety clamp
    servo1_target = constrain(servo1_target, 0, 90);
    servo2_target = constrain(servo2_target, 20, 160);
  }
}

void move_servos() {
  float servoSmoothing;

  if (emotion == NEUTRAL) {
    servoSmoothing = 0.08;   // was 0.04 — faster so head keeps up with tracking
  } else if (emotion == BREATHE) {
    servoSmoothing = 0.04;   // breathing stays very soft
  } else {
    servoSmoothing = 0.15;
  }

  servo1_pos += (servo1_target - servo1_pos) * servoSmoothing;
  servo2_pos += (servo2_target - servo2_pos) * servoSmoothing;

  servo1.write((int)servo1_pos);
  servo2.write((int)servo2_pos);
}

// ============================================================================
//  EARS
// ============================================================================
void update_ears() {
  if (emotion == lastEarEmotion) return;

  lastEarEmotion = emotion;

  switch (emotion) {
    case NEUTRAL:
  stopAllVibration();
  moveBothEars(LEFT_NEUTRAL, RIGHT_NEUTRAL);
  break;

case QUESTIONING:
  stopAllVibration();
  moveBothEars(LEFT_QUESTION, RIGHT_QUESTION);
  break;

case SAD:
  stopAllVibration();
  moveBothEars(LEFT_SAD, RIGHT_SAD);
  break;

case FROWN:
  stopAllVibration();
  moveBothEars(LEFT_FROWN, RIGHT_FROWN);
  break;

    case COMFORT:
      moveBothEars(LEFT_COMFORT, RIGHT_COMFORT);
      startPurr(PURR_DURATION_MS);
      break;

    case HAPPY:
      moveBothEars(LEFT_HAPPY, RIGHT_HAPPY);
      startPurr(PURR_DURATION_MS);
      break;

    case BREATHE:
      // Breathing ears and purr servo are animated continuously in update_breathing_motion().
      stopAllVibration();
      attachPurrServo();
      break;
  }
}

void moveBothEars(int leftTarget, int rightTarget) {
  int l = leftEar.read();
  int r = rightEar.read();

  int ls = (leftTarget  > l) ? 1 : -1;
  int rs = (rightTarget > r) ? 1 : -1;

  while (l != leftTarget || r != rightTarget) {
    if (l != leftTarget) {
      l += ls;
      leftEar.write(l);
    }

    if (r != rightTarget) {
      r += rs;
      rightEar.write(r);
    }

    delay(10);
  }
}

// ============================================================================
//  PURR SERVO + BREATHING MOTION
// ============================================================================
void attachPurrServo() {
  if (!purrServoAttached) {
    purrServo.attach(PURR_PIN);
    purrServoAttached = true;
  }
}

void detachPurrServo() {
  if (purrServoAttached) {
    purrServo.detach();
    purrServoAttached = false;
  }
}

void startPurr(unsigned long durationMs) {
  attachPurrServo();

  purr_active = true;
  purr_start_time = millis();
  purr_last_step = 0;
  purr_duration_ms = durationMs;
  purr_toggle = false;
}

void stopPurr() {
  purr_active = false;
  detachPurrServo();
}

void stopAllVibration() {
  // Important: do NOT keep writing PURR_CENTER here.
  // Repeated servo writes can make the servo twitch/jitter.
  // Detaching removes the servo signal completely.
  purr_active = false;
  detachPurrServo();
}

void update_purr_servo() {
  // Extra safety: the purr/vibration servo must be completely off
  // in Neutral, Questioning, Sad, and Frown.
  if (emotion == NEUTRAL ||
      emotion == QUESTIONING ||
      emotion == SAD ||
      emotion == FROWN) {
    stopAllVibration();
    return;
  }

  // During breathing, the purr servo is controlled slowly by
  // update_breathing_motion(), not by the fast purr wiggle.
  if (emotion == BREATHE) {
    attachPurrServo();
    return;
  }

  // Only Happy and Comfort are allowed to fast-purr.
  if (emotion != HAPPY && emotion != COMFORT) {
    stopAllVibration();
    return;
  }

  if (!purr_active) {
    return;
  }

  if (millis() - purr_start_time > purr_duration_ms) {
    stopAllVibration();
    return;
  }

  attachPurrServo();

  if (millis() - purr_last_step >= PURR_STEP_MS) {
    purr_last_step = millis();
    purr_toggle = !purr_toggle;

    if (purr_toggle) {
      purrServo.write(PURR_CENTER + PURR_RANGE);
    } else {
      purrServo.write(PURR_CENTER - PURR_RANGE);
    }
  }
}

int lerpInt(int startValue, int endValue, float amount) {
  amount = constrain(amount, 0.0, 1.0);
  return startValue + (long)(endValue - startValue) * amount;
}

void update_breathing_motion() {
  if (emotion != BREATHE) return;

  attachPurrServo();

  unsigned long totalBreathMs = BREATHE_IN_MS + BREATHE_HOLD_MS + BREATHE_OUT_MS;
  unsigned long elapsed = (millis() - breathing_start_time) % totalBreathMs;

  int leftPos;
  int rightPos;
  int purrPos;

  if (elapsed < BREATHE_IN_MS) {
    // Inhale: ears spread out, purr servo slowly moves outward.
    float amount = elapsed / (float)BREATHE_IN_MS;

    leftPos = lerpInt(LEFT_NEUTRAL, LEFT_BREATHE_IN, amount);
    rightPos = lerpInt(RIGHT_NEUTRAL, RIGHT_BREATHE_IN, amount);
    purrPos = lerpInt(PURR_CENTER, PURR_CENTER + PURR_BREATHE_RANGE, amount);

  } else if (elapsed < BREATHE_IN_MS + BREATHE_HOLD_MS) {
    // Hold breath: stay at the top.
    leftPos = LEFT_BREATHE_IN;
    rightPos = RIGHT_BREATHE_IN;
    purrPos = PURR_CENTER + PURR_BREATHE_RANGE;

  } else {
    // Exhale: ears and purr servo return to neutral.
    float amount = (elapsed - BREATHE_IN_MS - BREATHE_HOLD_MS) / (float)BREATHE_OUT_MS;

    leftPos = lerpInt(LEFT_BREATHE_IN, LEFT_BREATHE_OUT, amount);
    rightPos = lerpInt(RIGHT_BREATHE_IN, RIGHT_BREATHE_OUT, amount);
    purrPos = lerpInt(PURR_CENTER + PURR_BREATHE_RANGE, PURR_CENTER, amount);
  }

  leftEar.write(leftPos);
  rightEar.write(rightPos);
  purrServo.write(purrPos);
}

// ============================================================================
//  HUSKYLENS
// ============================================================================
void husky_lens() {
  if (!huskylens.request()) {
    // Keep previous state briefly if request fails.
  } else if (!huskylens.available()) {
    if (millis() - lastFaceSeen > 500) {
      face_detected = false;
    }
  } else {
    face_detected = false;

    int face_index = 0;

    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();

      if (result.command == COMMAND_RETURN_BLOCK) {
        if (face_index == 0 || result.ID == 1) {
          face = result;
        }

        face_index++;
        face_detected = true;
        lastFaceSeen = millis();
      }
    }
  }
}

// ============================================================================
//  EYES
// ============================================================================
void run_eyes() {
  leftEye.clear();
  rightEye.clear();

  // No blinking during breathing mode.
  if (emotion != BREATHE &&
      !do_blink &&
      millis() - last_blink_trigger > blink_interval) {

    trigger_blink();
    last_blink_trigger = millis();
    blink_interval = random(3500, 7000);
  }

  int hue = getEmotionHue(emotion);

  if (emotion == BREATHE && !do_blink) {
    LED_BRIGHTNESS = getBreathingBrightness();
  } else if (emotion == NEUTRAL) {
    LED_BRIGHTNESS = 3;   // neutral less bright
  } else {
    LED_BRIGHTNESS = 8;
  }

  // Blink override.
  if (do_blink) {
    unsigned long elapsed = millis() - blink_timer;

    if (elapsed < 100) {
      display_blink45(hue, 1);
    } else if (elapsed < 220) {
      display_blink45(hue, 2);
    } else if (elapsed < 340) {
      display_blink45(hue, 1);
    } else {
      do_blink = false;
    }

    leftEye.show();
    rightEye.show();
    return;
  }

  // Emotion display.
  switch (emotion) {
    case NEUTRAL:
      display_eyes(neutral, hue, LEFT_ROTATION_STEPS, RIGHT_ROTATION_STEPS);
      break;

    case QUESTIONING:
      display_eyes(questioning, hue, LEFT_ROTATION_STEPS, RIGHT_ROTATION_STEPS);
      break;

    case COMFORT:
      display_eyes(comfort, hue, LEFT_COMFORT_ROTATION_STEPS, RIGHT_COMFORT_ROTATION_STEPS);
      break;

    case HAPPY:
      display_eyes(happy, hue, LEFT_HAPPY_ROTATION_STEPS, RIGHT_HAPPY_ROTATION_STEPS);
      break;

    case SAD:
      display_eyes(sad, hue, LEFT_SAD_ROTATION_STEPS, RIGHT_SAD_ROTATION_STEPS);
      break;

    case FROWN:
      display_eyes(frown, hue, LEFT_FROWN_ROTATION_STEPS, RIGHT_FROWN_ROTATION_STEPS);
      break;

    case BREATHE:
      display_breathing_eyes(hue, LEFT_BREATHE_ROTATION_STEPS, RIGHT_BREATHE_ROTATION_STEPS);
      break;
  }

  leftEye.show();
  rightEye.show();
}

// ---------------- EMOTION COLORS ----------------
int getEmotionHue(int e) {
  switch (e) {
    case NEUTRAL:     return 110;
    case QUESTIONING: return 23;   // more orange questioning colour
    case COMFORT:     return 95;
    case HAPPY:       return 35;
    case SAD:         return 160;
    case FROWN:       return 5;
    case BREATHE:     return 145;
  }

  return 110;
}

// ---------------- BREATHING ANIMATION ----------------
float getBreathingWave() {
  unsigned long elapsed = millis() - breathing_start_time;

  // Hold the first moment at the minimum intensity.
  if (elapsed < BREATHING_START_HOLD_MS) {
    return 0.0;
  }

  unsigned long cycle = (elapsed - BREATHING_START_HOLD_MS) % BREATHING_CYCLE_MS;
  float phase = cycle / (float)BREATHING_CYCLE_MS;

  float wave = (sin(phase * TWO_PI - HALF_PI) + 1.0) / 2.0;

  return wave;
}

int getBreathingBrightness() {
  float wave = getBreathingWave();

  return BREATHING_MIN_BRIGHTNESS + wave * (BREATHING_MAX_BRIGHTNESS - BREATHING_MIN_BRIGHTNESS);
}

// ---------------- BLINK TRIGGER ----------------
void trigger_blink() {
  do_blink = true;
  blink_timer = millis();
}

// ---------------- HEX HELPERS ----------------
HexCoord indexToCoord(int index) {
  int counter = 0;

  for (int row = 0; row < 7; row++) {
    int r = row - 3;
    int qStart = max(-3, -r - 3);

    for (int col = 0; col < rows[row]; col++) {
      if (counter == index) {
        HexCoord c = { qStart + col, r };
        return c;
      }

      counter++;
    }
  }

  HexCoord fallback = { 0, 0 };
  return fallback;
}

int coordToIndex(HexCoord c) {
  int counter = 0;

  for (int row = 0; row < 7; row++) {
    int r = row - 3;
    int qStart = max(-3, -r - 3);

    for (int col = 0; col < rows[row]; col++) {
      int q = qStart + col;

      if (q == c.q && r == c.r) {
        return counter;
      }

      counter++;
    }
  }

  return 0;
}

HexCoord rotateHex120(HexCoord c) {
  HexCoord rotated = { -c.q - c.r, c.q };
  return rotated;
}

HexCoord rotateHex240(HexCoord c) {
  HexCoord rotated = { c.r, -c.q - c.r };
  return rotated;
}

HexCoord applyHexRotation(HexCoord c, int steps) {
  steps = steps % 3;

  if (steps < 0) {
    steps += 3;
  }

  if (steps == 1) {
    return rotateHex120(c);
  } else if (steps == 2) {
    return rotateHex240(c);
  }

  return c;
}

int getHexRing(HexCoord c) {
  int s = -c.q - c.r;

  int aq = abs(c.q);
  int ar = abs(c.r);
  int as = abs(s);

  return max(aq, max(ar, as));
}

// ---------------- DRAW BOTH NORMAL EYES ----------------
void display_eyes(byte arr[], int hue, int leftRotation, int rightRotation) {
  display_eye(leftEye, arr, hue, true, leftRotation);
  display_eye(rightEye, arr, hue, false, rightRotation);
}

// ---------------- DRAW ONE NORMAL EYE ----------------
void display_eye(Adafruit_NeoPixel &eye, byte arr[], int hue, bool isLeftEye, int rotationSteps) {
  int index = 0;

  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < rows[row]; col++) {
      int bitValue = bitRead(arr[row], isLeftEye ? rows[row] - 1 - col : col);

      if (bitValue == 1) {
        setMappedPixel(eye, index, hue, LED_BRIGHTNESS, isLeftEye, rotationSteps);
      }

      index++;
    }
  }
}

// ---------------- BREATHING CIRCLE ANIMATION ----------------
void display_breathing_eyes(int hue, int leftRotation, int rightRotation) {
  display_breathing_eye(leftEye, hue, true, leftRotation);
  display_breathing_eye(rightEye, hue, false, rightRotation);
}

void display_breathing_eye(Adafruit_NeoPixel &eye, int hue, bool isLeftEye, int rotationSteps) {
  float wave = getBreathingWave();

  // Starts with 7 LEDs in the middle:
  // ring 0 = center LED
  // ring 1 = 6 LEDs around center
  float breathingRadius = 1.0 + wave * 2.0;

  int baseBrightness = getBreathingBrightness();

  for (int index = 0; index < PIXELS_PER_EYE; index++) {
    HexCoord coord = indexToCoord(index);
    int ring = getHexRing(coord);

    float ringFill = breathingRadius - ring + 1.0;

    if (ringFill < 0.0) ringFill = 0.0;
    if (ringFill > 1.0) ringFill = 1.0;

    int brightness = baseBrightness * ringFill;

    if (brightness > 0) {
      setMappedPixel(eye, index, hue, brightness, isLeftEye, rotationSteps);
    }
  }
}

// ---------------- 45-DEGREE BLINK ----------------
void display_blink45(int hue, int blinkStage) {
  display_blink45_eye(leftEye, hue, blinkStage, true, LEFT_BLINK_ROTATION_STEPS);
  display_blink45_eye(rightEye, hue, blinkStage, false, RIGHT_BLINK_ROTATION_STEPS);
}

void display_blink45_eye(Adafruit_NeoPixel &eye, int hue, int blinkStage, bool isLeftEye, int rotationSteps) {
  int index = 0;

  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < rows[row]; col++) {
      int patternCol = isLeftEye ? rows[row] - 1 - col : col;

      int diagonalCol;

      if (BLINK45_DIRECTION == 1) {
        diagonalCol = (row * (rows[row] - 1) + 3) / 6;
      } else {
        diagonalCol = ((6 - row) * (rows[row] - 1) + 3) / 6;
      }

      int thickness;

      if (blinkStage == 1) {
        thickness = 1;
      } else {
        thickness = 0;
      }

      int bitValue = abs(patternCol - diagonalCol) <= thickness;

      if (bitValue == 1) {
        setMappedPixel(eye, index, hue, LED_BRIGHTNESS, isLeftEye, rotationSteps);
      }

      index++;
    }
  }
}

// ---------------- PIXEL MAPPING ----------------
void setMappedPixel(Adafruit_NeoPixel &eye, int index, int hue, int brightness, bool isLeftEye, int rotationSteps) {
  HexCoord coord = indexToCoord(index);

  coord = applyHexRotation(coord, rotationSteps);

  int ledIndex = coordToIndex(coord);

  if (!isLeftEye && RIGHT_REVERSED) {
    ledIndex = PIXELS_PER_EYE - 1 - ledIndex;
  }

  eye.setPixelColor(
    ledIndex,
    eye.ColorHSV(hue * 256, 255, brightness)
  );
}