/*
   ============================================================
                    DASAI MOCHI
               EMO-INSPIRED EYE SYSTEM

   ESP32 + 2x SSD1306 OLED + DHT11
   + Push Button + Bluetooth + WiFi/NTP

   ------------------------------------------------------------
   OLED 1 - STATUS
   SDA -> GPIO 21
   SCL -> GPIO 22

   OLED 2 - EMO FACE
   SDA -> GPIO 25
   SCL -> GPIO 26

   DHT11
   DATA -> GPIO 18

   BUTTON
   GPIO 19 -> GND

   BLUETOOTH
   Dasai_Mochi

   ------------------------------------------------------------
   FACE DESIGN:

   NO EYEBROWS
   NO PUPILS
   NO DARK EYES

   The eyes themselves change shape like an
   EMO-style digital pet display.
   ============================================================
*/

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include "BluetoothSerial.h"
#include "time.h"

// ============================================================
// WIFI
// ============================================================

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const long GMT_OFFSET_SEC = 19800;
const int DAYLIGHT_OFFSET_SEC = 0;

const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.nist.gov";

// ============================================================
// OLED
// ============================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

// STATUS OLED
#define INFO_SDA 21
#define INFO_SCL 22

// FACE OLED
#define FACE_SDA 25
#define FACE_SCL 26

Adafruit_SSD1306 displayInfo(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

Adafruit_SSD1306 displayFace(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire1,
  OLED_RESET
);

// ============================================================
// DHT11
// ============================================================

#define DHT_PIN 18
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
// BUTTON
// ============================================================

#define BUTTON_PIN 19

// ============================================================
// BLUETOOTH
// ============================================================

BluetoothSerial SerialBT;

// ============================================================
// MOODS
// ============================================================

enum Mood {

  MOOD_NORMAL,
  MOOD_HAPPY,
  MOOD_SAD,
  MOOD_ANGRY,
  MOOD_SURPRISE,
  MOOD_SLEEP
};

Mood currentMood = MOOD_NORMAL;

// ============================================================
// EMO EYE SHAPE
// ============================================================

struct EyeShape {

  float width;
  float height;

  float centerY;

  float tilt;

  float roundness;

  float upperCut;

  float lowerCut;
};

// Current eye shape
EyeShape leftEye;
EyeShape rightEye;

// Target eye shape
EyeShape targetEye;

// ============================================================
// EYE MOVEMENT
// ============================================================

float gazeX = 0;
float gazeY = 0;

float targetGazeX = 0;
float targetGazeY = 0;

unsigned long nextGaze = 0;

// ============================================================
// BLINK
// ============================================================

bool blinking = false;

float blinkAmount = 0;

unsigned long blinkStart = 0;

unsigned long nextBlink = 0;

const int BLINK_DURATION = 170;

// ============================================================
// NATURAL MOTION
// ============================================================

float idleMotion = 0;

unsigned long lastFrame = 0;

const int FRAME_TIME = 20;

// ============================================================
// RESPONSE
// ============================================================

String faceMessage =
  "Hello! I am Mochi.";

unsigned long messageUntil = 0;

const unsigned long MESSAGE_DURATION = 4500;

// ============================================================
// DHT
// ============================================================

float temperature = 0;
float humidity = 0;

unsigned long lastDHT = 0;

const unsigned long DHT_INTERVAL = 2500;

// ============================================================
// STATUS DISPLAY
// ============================================================

unsigned long lastInfo = 0;

const unsigned long INFO_INTERVAL = 500;

// ============================================================
// BUTTON
// ============================================================

bool lastButtonState = HIGH;

unsigned long lastButtonTime = 0;

const unsigned long BUTTON_DEBOUNCE = 250;

// ============================================================
// FUNCTION DECLARATIONS
// ============================================================

void connectWiFi();
void readDHT();

void handleBluetooth();
void handleButton();

void updateFace();
void drawFace();

void setMood(Mood mood);
void respond(String text);

void updateMoodTarget();

void gazeSystem();
void blinkSystem();

void drawEmoEyes();
void drawSingleEye(float cx, float cy, EyeShape shape);

void drawEyePolygon(
  float cx,
  float cy,
  float w,
  float h,
  float tilt,
  float roundness,
  float upperCut,
  float lowerCut
);

void drawResponse();

String getTimeString();
String getDateString();

void updateInfoDisplay();


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  // ----------------------------------------------------------
  // STATUS OLED
  // ----------------------------------------------------------

  Wire.begin(
    INFO_SDA,
    INFO_SCL
  );

  // ----------------------------------------------------------
  // FACE OLED
  // ----------------------------------------------------------

  Wire1.begin(
    FACE_SDA,
    FACE_SCL
  );

  // ----------------------------------------------------------
  // OLED 1
  // ----------------------------------------------------------

  if (!displayInfo.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {

    Serial.println("STATUS OLED ERROR");
  }

  // ----------------------------------------------------------
  // OLED 2
  // ----------------------------------------------------------

  if (!displayFace.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {

    Serial.println("FACE OLED ERROR");
  }

  displayInfo.clearDisplay();
  displayInfo.display();

  displayFace.clearDisplay();
  displayFace.display();

  // ----------------------------------------------------------
  // DHT
  // ----------------------------------------------------------

  dht.begin();

  // ----------------------------------------------------------
  // BUTTON
  // ----------------------------------------------------------

  pinMode(
    BUTTON_PIN,
    INPUT_PULLUP
  );

  // ----------------------------------------------------------
  // BLUETOOTH
  // ----------------------------------------------------------

  SerialBT.begin(
    "Dasai_Mochi"
  );

  SerialBT.setTimeout(30);

  // ----------------------------------------------------------
  // RANDOM
  // ----------------------------------------------------------

  randomSeed(
    analogRead(34)
  );

  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  connectWiFi();

  readDHT();

  // ----------------------------------------------------------
  // INITIAL FACE
  // ----------------------------------------------------------

  leftEye.width = 28;
  leftEye.height = 30;
  leftEye.centerY = 24;
  leftEye.tilt = 0;
  leftEye.roundness = 7;
  leftEye.upperCut = 0;
  leftEye.lowerCut = 0;

  rightEye = leftEye;

  targetEye = leftEye;

  nextBlink =
    millis() + random(1800, 4500);

  nextGaze =
    millis() + random(1200, 3000);

  respond(
    "Hello! I am Mochi."
  );

  Serial.println();
  Serial.println(
    "=============================="
  );

  Serial.println(
    "       DASAI MOCHI READY"
  );

  Serial.println(
    "=============================="
  );
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  unsigned long now = millis();

  handleBluetooth();

  handleButton();

  // ----------------------------------------------------------
  // DHT
  // ----------------------------------------------------------

  if (
    now - lastDHT >= DHT_INTERVAL
  ) {

    readDHT();

    lastDHT = now;
  }

  // ----------------------------------------------------------
  // FACE
  // ----------------------------------------------------------

  if (
    now - lastFrame >= FRAME_TIME
  ) {

    lastFrame = now;

    gazeSystem();

    blinkSystem();

    updateFace();

    drawFace();
  }

  // ----------------------------------------------------------
  // STATUS
  // ----------------------------------------------------------

  if (
    now - lastInfo >= INFO_INTERVAL
  ) {

    lastInfo = now;

    updateInfoDisplay();
  }
}


// ============================================================
// WIFI
// ============================================================

void connectWiFi() {

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.print(
    "Connecting WiFi"
  );

  unsigned long start =
    millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - start < 10000
  ) {

    delay(300);

    Serial.print(".");
  }

  Serial.println();

  if (
    WiFi.status() == WL_CONNECTED
  ) {

    Serial.println(
      "WiFi connected"
    );

    configTime(
      GMT_OFFSET_SEC,
      DAYLIGHT_OFFSET_SEC,
      NTP_SERVER_1,
      NTP_SERVER_2
    );
  }
  else {

    Serial.println(
      "Offline mode"
    );
  }
}


// ============================================================
// DHT
// ============================================================

void readDHT() {

  float h =
    dht.readHumidity();

  float t =
    dht.readTemperature();

  if (
    !isnan(h) &&
    !isnan(t)
  ) {

    humidity = h;

    temperature = t;
  }
}


// ============================================================
// TIME
// ============================================================

String getTimeString() {

  struct tm timeinfo;

  if (
    !getLocalTime(
      &timeinfo,
      50
    )
  ) {

    return "--:--:--";
  }

  char buffer[16];

  strftime(
    buffer,
    sizeof(buffer),
    "%H:%M:%S",
    &timeinfo
  );

  return String(buffer);
}


// ============================================================
// DATE
// ============================================================

String getDateString() {

  struct tm timeinfo;

  if (
    !getLocalTime(
      &timeinfo,
      50
    )
  ) {

    return "--/--/----";
  }

  char buffer[16];

  strftime(
    buffer,
    sizeof(buffer),
    "%d/%m/%Y",
    &timeinfo
  );

  return String(buffer);
}


// ============================================================
// MOOD
// ============================================================

void setMood(Mood mood) {

  currentMood = mood;

  // Interaction causes Mochi to look toward user
  targetGazeX = 0;
  targetGazeY = 0;

  nextGaze =
    millis() + random(1500, 3500);
}


// ============================================================
// RESPONSE
// ============================================================

void respond(String text) {

  faceMessage = text;

  messageUntil =
    millis() + MESSAGE_DURATION;

  SerialBT.println(text);

  Serial.print(
    "Mochi: "
  );

  Serial.println(text);
}


// ============================================================
// BLUETOOTH
// ============================================================

void handleBluetooth() {

  if (!SerialBT.available()) {

    return;
  }

  String cmd =
    SerialBT.readStringUntil('\n');

  cmd.trim();

  if (cmd.length() == 0) {

    return;
  }

  cmd.toUpperCase();

  Serial.print(
    "Command: "
  );

  Serial.println(cmd);

  // ----------------------------------------------------------
  // HELP
  // ----------------------------------------------------------

  if (
    cmd == "HELP" ||
    cmd == "COMMANDS"
  ) {

    respond(
      "Try HI, LOVE, TIME or TEMP"
    );

    SerialBT.println(
      "Commands:"
    );

    SerialBT.println(
      "HI / HELLO"
    );

    SerialBT.println(
      "HOW ARE YOU"
    );

    SerialBT.println(
      "LOVE YOU"
    );

    SerialBT.println(
      "TIME"
    );

    SerialBT.println(
      "DATE"
    );

    SerialBT.println(
      "TEMP"
    );

    SerialBT.println(
      "HUMIDITY"
    );

    SerialBT.println(
      "STATUS"
    );

    SerialBT.println(
      "HAPPY / SAD"
    );

    SerialBT.println(
      "ANGRY / SURPRISE"
    );

    SerialBT.println(
      "SLEEP / NORMAL"
    );

    return;
  }

  // ----------------------------------------------------------
  // HI
  // ----------------------------------------------------------

  if (
    cmd == "HI" ||
    cmd == "HELLO" ||
    cmd == "HEY" ||
    cmd == "HII"
  ) {

    setMood(MOOD_HAPPY);

    respond(
      "Hey! Good to see you!"
    );

    return;
  }

  // ----------------------------------------------------------
  // HOW ARE YOU
  // ----------------------------------------------------------

  if (
    cmd.indexOf("HOW ARE YOU") >= 0 ||
    cmd.indexOf("ARE YOU OK") >= 0
  ) {

    setMood(MOOD_HAPPY);

    respond(
      "I feel great!"
    );

    return;
  }

  // ----------------------------------------------------------
  // NAME
  // ----------------------------------------------------------

  if (
    cmd.indexOf("YOUR NAME") >= 0 ||
    cmd == "WHO ARE YOU"
  ) {

    setMood(MOOD_HAPPY);

    respond(
      "I am Mochi!"
    );

    return;
  }

  // ----------------------------------------------------------
  // LOVE
  // ----------------------------------------------------------

  if (
    cmd.indexOf("LOVE") >= 0
  ) {

    setMood(MOOD_HAPPY);

    respond(
      "I love you too!"
    );

    return;
  }

  // ----------------------------------------------------------
  // MISS YOU
  // ----------------------------------------------------------

  if (
    cmd.indexOf("MISS YOU") >= 0
  ) {

    setMood(MOOD_SAD);

    respond(
      "I missed you too..."
    );

    return;
  }

  // ----------------------------------------------------------
  // THANKS
  // ----------------------------------------------------------

  if (
    cmd.indexOf("THANK") >= 0
  ) {

    setMood(MOOD_HAPPY);

    respond(
      "You are welcome!"
    );

    return;
  }

  // ----------------------------------------------------------
  // GOOD MORNING
  // ----------------------------------------------------------

  if (
    cmd.indexOf("GOOD MORNING") >= 0
  ) {

    setMood(MOOD_HAPPY);

    respond(
      "Good morning!"
    );

    return;
  }

  // ----------------------------------------------------------
  // GOOD NIGHT
  // ----------------------------------------------------------

  if (
    cmd.indexOf("GOOD NIGHT") >= 0
  ) {

    setMood(MOOD_SLEEP);

    respond(
      "Good night... Zzz"
    );

    return;
  }

  // ----------------------------------------------------------
  // BYE
  // ----------------------------------------------------------

  if (
    cmd == "BYE" ||
    cmd == "GOODBYE"
  ) {

    setMood(MOOD_SLEEP);

    respond(
      "Bye! Come back soon."
    );

    return;
  }

  // ----------------------------------------------------------
  // HAPPY
  // ----------------------------------------------------------

  if (
    cmd == "HAPPY"
  ) {

    setMood(MOOD_HAPPY);

    respond(
      "Yay! I am happy!"
    );

    return;
  }

  // ----------------------------------------------------------
  // SAD
  // ----------------------------------------------------------

  if (
    cmd == "SAD"
  ) {

    setMood(MOOD_SAD);

    respond(
      "Aww... I am sad."
    );

    return;
  }

  // ----------------------------------------------------------
  // ANGRY
  // ----------------------------------------------------------

  if (
    cmd == "ANGRY"
  ) {

    setMood(MOOD_ANGRY);

    respond(
      "Grrr... Mochi angry!"
    );

    return;
  }

  // ----------------------------------------------------------
  // SURPRISE
  // ----------------------------------------------------------

  if (
    cmd == "SURPRISE" ||
    cmd == "WOW"
  ) {

    setMood(MOOD_SURPRISE);

    respond(
      "Whoa! Really?"
    );

    return;
  }

  // ----------------------------------------------------------
  // SLEEP
  // ----------------------------------------------------------

  if (
    cmd == "SLEEP"
  ) {

    setMood(MOOD_SLEEP);

    respond(
      "I am sleepy... Zzz"
    );

    return;
  }

  // ----------------------------------------------------------
  // NORMAL
  // ----------------------------------------------------------

  if (
    cmd == "NORMAL" ||
    cmd == "RESET"
  ) {

    setMood(MOOD_NORMAL);

    respond(
      "Back to normal."
    );

    return;
  }

  // ----------------------------------------------------------
  // TIME
  // ----------------------------------------------------------

  if (
    cmd.indexOf("TIME") >= 0
  ) {

    setMood(MOOD_NORMAL);

    respond(
      "Time " + getTimeString()
    );

    return;
  }

  // ----------------------------------------------------------
  // DATE
  // ----------------------------------------------------------

  if (
    cmd.indexOf("DATE") >= 0 ||
    cmd == "TODAY"
  ) {

    setMood(MOOD_NORMAL);

    respond(
      "Date " + getDateString()
    );

    return;
  }

  // ----------------------------------------------------------
  // TEMPERATURE
  // ----------------------------------------------------------

  if (
    cmd.indexOf("TEMP") >= 0 ||
    cmd.indexOf("TEMPERATURE") >= 0
  ) {

    respond(
      "Temp " +
      String(temperature, 1) +
      " C"
    );

    return;
  }

  // ----------------------------------------------------------
  // HUMIDITY
  // ----------------------------------------------------------

  if (
    cmd.indexOf("HUMIDITY") >= 0
  ) {

    respond(
      "Humidity " +
      String(humidity, 0) +
      "%"
    );

    return;
  }

  // ----------------------------------------------------------
  // WEATHER
  // ----------------------------------------------------------

  if (
    cmd.indexOf("WEATHER") >= 0
  ) {

    respond(
      "Temp " +
      String(temperature, 1) +
      "C Hum " +
      String(humidity, 0) +
      "%"
    );

    return;
  }

  // ----------------------------------------------------------
  // STATUS
  // ----------------------------------------------------------

  if (
    cmd == "STATUS"
  ) {

    respond(
      "T:" +
      String(temperature, 1) +
      "C H:" +
      String(humidity, 0) +
      "%"
    );

    SerialBT.println(
      "Mochi status OK"
    );

    SerialBT.println(
      "Time: " + getTimeString()
    );

    SerialBT.println(
      "Date: " + getDateString()
    );

    return;
  }

  // ----------------------------------------------------------
  // JOKE
  // ----------------------------------------------------------

  if (
    cmd == "JOKE"
  ) {

    setMood(MOOD_HAPPY);

    respond(
      "I blink because I am cute!"
    );

    return;
  }

  // ----------------------------------------------------------
  // UNKNOWN
  // ----------------------------------------------------------

  setMood(MOOD_SURPRISE);

  respond(
    "Hmm... I don't know that."
  );
}


// ============================================================
// BUTTON
// ============================================================

void handleButton() {

  bool state =
    digitalRead(BUTTON_PIN);

  if (
    lastButtonState == HIGH &&
    state == LOW
  ) {

    if (
      millis() - lastButtonTime >
      BUTTON_DEBOUNCE
    ) {

      lastButtonTime =
        millis();

      setMood(MOOD_HAPPY);

      targetGazeX = 0;
      targetGazeY = -3;

      respond(
        "You touched me!"
      );
    }
  }

  lastButtonState = state;
}


// ============================================================
// GAZE SYSTEM
// ============================================================

void gazeSystem() {

  unsigned long now =
    millis();

  if (
    now >= nextGaze
  ) {

    int choice =
      random(0, 10);

    if (choice == 0) {

      targetGazeX = -7;
      targetGazeY = 0;
    }

    else if (choice == 1) {

      targetGazeX = 7;
      targetGazeY = 0;
    }

    else if (choice == 2) {

      targetGazeX = 0;
      targetGazeY = -4;
    }

    else if (choice == 3) {

      targetGazeX = 0;
      targetGazeY = 4;
    }

    else {

      targetGazeX =
        random(-2, 3);

      targetGazeY =
        random(-2, 3);
    }

    nextGaze =
      now + random(1200, 3500);
  }

  // Very smooth eye movement
  gazeX +=
    (targetGazeX - gazeX) * 0.075;

  gazeY +=
    (targetGazeY - gazeY) * 0.075;
}


// ============================================================
// BLINK SYSTEM
// ============================================================

void blinkSystem() {

  unsigned long now =
    millis();

  // Sleep = eyes stay closed
  if (
    currentMood == MOOD_SLEEP
  ) {

    blinkAmount +=
      (1.0 - blinkAmount) * 0.12;

    return;
  }

  // Start blink
  if (
    !blinking &&
    now >= nextBlink
  ) {

    blinking = true;

    blinkStart = now;

    nextBlink =
      now + random(2200, 5500);
  }

  // Animate blink
  if (blinking) {

    unsigned long elapsed =
      now - blinkStart;

    // Closing
    if (
      elapsed < BLINK_DURATION / 2
    ) {

      float p =
        (float)elapsed /
        (BLINK_DURATION / 2);

      // Smoothstep
      blinkAmount =
        p * p * (3.0 - 2.0 * p);
    }

    // Opening
    else if (
      elapsed < BLINK_DURATION
    ) {

      float p =
        (float)(
          elapsed -
          BLINK_DURATION / 2
        ) /
        (BLINK_DURATION / 2);

      p = 1.0 - p;

      blinkAmount =
        p * p * (3.0 - 2.0 * p);
    }

    else {

      blinkAmount = 0;

      blinking = false;
    }
  }
}


// ============================================================
// MOOD TARGET
// ============================================================

void updateMoodTarget() {

  // Start with normal EMO eyes

  targetEye.width = 28;
  targetEye.height = 30;

  targetEye.centerY = 24;

  targetEye.tilt = 0;

  targetEye.roundness = 7;

  targetEye.upperCut = 0;

  targetEye.lowerCut = 0;

  switch (currentMood) {

    // ========================================================
    // NORMAL
    // ========================================================

    case MOOD_NORMAL:

      targetEye.width = 28;
      targetEye.height = 30;

      targetEye.centerY = 24;

      targetEye.roundness = 7;

      break;


    // ========================================================
    // HAPPY
    // ========================================================

    case MOOD_HAPPY:

      /*
         EMO-style happy expression:

              ______    ______
             /      \  /      \
             \______/  \______/

         Eyes become shorter and curved.
      */

      targetEye.width = 30;

      targetEye.height = 22;

      targetEye.centerY = 25;

      targetEye.roundness = 9;

      targetEye.lowerCut = 9;

      break;


    // ========================================================
    // SAD
    // ========================================================

    case MOOD_SAD:

      /*
         Droopy eyes
      */

      targetEye.width = 27;

      targetEye.height = 27;

      targetEye.centerY = 26;

      targetEye.roundness = 8;

      targetEye.upperCut = 5;

      break;


    // ========================================================
    // ANGRY
    // ========================================================

    case MOOD_ANGRY:

      /*
         Angled aggressive eyes
      */

      targetEye.width = 30;

      targetEye.height = 21;

      targetEye.centerY = 24;

      targetEye.tilt = 7;

      targetEye.roundness = 6;

      break;


    // ========================================================
    // SURPRISE
    // ========================================================

    case MOOD_SURPRISE:

      targetEye.width = 30;

      targetEye.height = 35;

      targetEye.centerY = 23;

      targetEye.roundness = 10;

      break;


    // ========================================================
    // SLEEP
    // ========================================================

    case MOOD_SLEEP:

      targetEye.width = 28;

      targetEye.height = 4;

      targetEye.centerY = 27;

      targetEye.roundness = 2;

      break;
  }
}


// ============================================================
// SMOOTH FACE UPDATE
// ============================================================

void updateFace() {

  updateMoodTarget();

  // ----------------------------------------------------------
  // Smoothly morph eye shape
  // ----------------------------------------------------------

  leftEye.width +=
    (targetEye.width -
     leftEye.width) * 0.10;

  leftEye.height +=
    (targetEye.height -
     leftEye.height) * 0.10;

  leftEye.centerY +=
    (targetEye.centerY -
     leftEye.centerY) * 0.10;

  leftEye.tilt +=
    (targetEye.tilt -
     leftEye.tilt) * 0.10;

  leftEye.roundness +=
    (targetEye.roundness -
     leftEye.roundness) * 0.10;

  leftEye.upperCut +=
    (targetEye.upperCut -
     leftEye.upperCut) * 0.10;

  leftEye.lowerCut +=
    (targetEye.lowerCut -
     leftEye.lowerCut) * 0.10;


  // Right eye follows same expression
  rightEye = leftEye;
}


// ============================================================
// DRAW FACE
// ============================================================

void drawFace() {

  displayFace.clearDisplay();

  // ----------------------------------------------------------
  // EMO EYES
  // ----------------------------------------------------------

  drawEmoEyes();

  // ----------------------------------------------------------
  // DIVIDER
  // ----------------------------------------------------------

  displayFace.drawLine(
    0,
    44,
    127,
    44,
    SSD1306_WHITE
  );

  // ----------------------------------------------------------
  // RESPONSE
  // ----------------------------------------------------------

  drawResponse();

  displayFace.display();
}


// ============================================================
// DRAW EMO EYES
// ============================================================

void drawEmoEyes() {

  /*
        OLED FACE

       +----------------------+
       |                      |
       |    █████    █████    |
       |   ███████  ███████   |
       |   ███████  ███████   |
       |    █████    █████    |
       |                      |
       |----------------------|
       |    Mochi response    |
       +----------------------+

       NO PUPILS
       NO EYEBROWS
  */

  float centerLeft =
    38 + gazeX;

  float centerRight =
    90 + gazeX;

  float y =
    leftEye.centerY +
    gazeY +
    idleMotion;

  // ----------------------------------------------------------
  // Draw left eye
  // ----------------------------------------------------------

  drawSingleEye(
    centerLeft,
    y,
    leftEye
  );

  // ----------------------------------------------------------
  // Draw right eye
  // ----------------------------------------------------------

  drawSingleEye(
    centerRight,
    y,
    rightEye
  );
}


// ============================================================
// SINGLE EYE
// ============================================================

void drawSingleEye(
  float cx,
  float cy,
  EyeShape shape
) {

  float finalHeight =
    shape.height *
    (1.0 - blinkAmount);

  if (finalHeight < 2) {

    finalHeight = 2;
  }

  drawEyePolygon(
    cx,
    cy,
    shape.width,
    finalHeight,
    shape.tilt,
    shape.roundness,
    shape.upperCut,
    shape.lowerCut
  );
}


// ============================================================
// EMO EYE POLYGON
// ============================================================

void drawEyePolygon(
  float cx,
  float cy,
  float w,
  float h,
  float tilt,
  float roundness,
  float upperCut,
  float lowerCut
) {

  /*
     Create a filled rounded eye shape.

     The polygon is made from many points.
     This allows the eye to smoothly morph
     between expressions.
  */

  const int POINTS = 20;

  int16_t px[POINTS];
  int16_t py[POINTS];

  float rx =
    w / 2.0;

  float ry =
    h / 2.0;

  // ----------------------------------------------------------
  // Build rounded rectangle perimeter
  // ----------------------------------------------------------

  for (
    int i = 0;
    i < POINTS;
    i++
  ) {

    float angle =
      (2.0 * PI * i) /
      POINTS;

    float x =
      cos(angle) * rx;

    float y =
      sin(angle) * ry;

    // --------------------------------------------------------
    // Round/smooth shape
    // --------------------------------------------------------

    float roundFactor =
      1.0;

    if (
      fabs(x) > rx - roundness
    ) {

      roundFactor *= 0.96;
    }

    if (
      fabs(y) > ry - roundness
    ) {

      roundFactor *= 0.96;
    }

    x *= roundFactor;
    y *= roundFactor;

    // --------------------------------------------------------
    // Angry tilt
    // --------------------------------------------------------

    float tiltOffset =
      (x / rx) * tilt;

    y += tiltOffset;

    // --------------------------------------------------------
    // Sad upper cut
    // --------------------------------------------------------

    if (
      upperCut > 0 &&
      y < 0
    ) {

      float amount =
        upperCut *
        (1.0 - fabs(x) / rx);

      y += amount;
    }

    // --------------------------------------------------------
    // Happy lower cut
    // --------------------------------------------------------

    if (
      lowerCut > 0 &&
      y > 0
    ) {

      float amount =
        lowerCut *
        (1.0 - fabs(x) / rx);

      y -= amount;
    }

    px[i] =
      (int16_t)(cx + x);

    py[i] =
      (int16_t)(cy + y);
  }

  // ----------------------------------------------------------
  // Fill polygon using triangles
  // ----------------------------------------------------------

  for (
    int i = 0;
    i < POINTS;
    i++
  ) {

    int next =
      (i + 1) % POINTS;

    displayFace.fillTriangle(
      (int16_t)cx,
      (int16_t)cy,
      px[i],
      py[i],
      px[next],
      py[next],
      SSD1306_WHITE
    );
  }
}


// ============================================================
// RESPONSE TEXT
// ============================================================

void drawResponse() {

  displayFace.setTextSize(1);

  displayFace.setTextColor(
    SSD1306_WHITE
  );

  String msg =
    faceMessage;

  const int MAX_CHARS = 20;

  String line1 = "";
  String line2 = "";

  // ----------------------------------------------------------
  // One line
  // ----------------------------------------------------------

  if (
    msg.length() <= MAX_CHARS
  ) {

    line1 = msg;
  }

  // ----------------------------------------------------------
  // Two lines
  // ----------------------------------------------------------

  else {

    int cut =
      msg.lastIndexOf(
        ' ',
        MAX_CHARS
      );

    if (cut <= 0) {

      cut = MAX_CHARS;
    }

    line1 =
      msg.substring(
        0,
        cut
      );

    line2 =
      msg.substring(
        cut + 1
      );

    if (
      line2.length() > MAX_CHARS
    ) {

      line2 =
        line2.substring(
          0,
          MAX_CHARS - 3
        );

      line2 += "...";
    }
  }

  // ----------------------------------------------------------
  // Center first line
  // ----------------------------------------------------------

  int width1 =
    line1.length() * 6;

  int x1 =
    (128 - width1) / 2;

  if (x1 < 0)
    x1 = 0;

  displayFace.setCursor(
    x1,
    47
  );

  displayFace.print(
    line1
  );

  // ----------------------------------------------------------
  // Second line
  // ----------------------------------------------------------

  if (
    line2.length() > 0
  ) {

    int width2 =
      line2.length() * 6;

    int x2 =
      (128 - width2) / 2;

    if (x2 < 0)
      x2 = 0;

    displayFace.setCursor(
      x2,
      56
    );

    displayFace.print(
      line2
    );
  }
}


// ============================================================
// STATUS OLED
// ============================================================

void updateInfoDisplay() {

  displayInfo.clearDisplay();

  displayInfo.setTextColor(
    SSD1306_WHITE
  );

  // ----------------------------------------------------------
  // TIME
  // ----------------------------------------------------------

  displayInfo.setTextSize(2);

  String time =
    getTimeString();

  int timeX =
    (128 - time.length() * 12) / 2;

  displayInfo.setCursor(
    timeX,
    1
  );

  displayInfo.print(
    time
  );

  // ----------------------------------------------------------
  // DATE
  // ----------------------------------------------------------

  displayInfo.setTextSize(1);

  String date =
    getDateString();

  int dateX =
    (128 - date.length() * 6) / 2;

  displayInfo.setCursor(
    dateX,
    22
  );

  displayInfo.print(
    date
  );

  // ----------------------------------------------------------
  // TEMPERATURE
  // ----------------------------------------------------------

  displayInfo.setCursor(
    3,
    36
  );

  displayInfo.print(
    "TEMP:"
  );

  displayInfo.print(
    temperature,
    1
  );

  displayInfo.print(
    "C"
  );

  // ----------------------------------------------------------
  // HUMIDITY
  // ----------------------------------------------------------

  displayInfo.setCursor(
    69,
    36
  );

  displayInfo.print(
    "HUM:"
  );

  displayInfo.print(
    humidity,
    0
  );

  displayInfo.print(
    "%"
  );

  // ----------------------------------------------------------
  // MOOD
  // ----------------------------------------------------------

  displayInfo.setCursor(
    3,
    52
  );

  displayInfo.print(
    "MOCHI:"
  );

  switch (currentMood) {

    case MOOD_NORMAL:
      displayInfo.print("NORMAL");
      break;

    case MOOD_HAPPY:
      displayInfo.print("HAPPY");
      break;

    case MOOD_SAD:
      displayInfo.print("SAD");
      break;

    case MOOD_ANGRY:
      displayInfo.print("ANGRY");
      break;

    case MOOD_SURPRISE:
      displayInfo.print("SURPRISE");
      break;

    case MOOD_SLEEP:
      displayInfo.print("SLEEP");
      break;
  }

  displayInfo.display();
}
