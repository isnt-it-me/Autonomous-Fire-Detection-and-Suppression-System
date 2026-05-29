#include <ESP32Servo.h>

#define IR_PIN 27
#define MQ2_PIN 34
#define MQ135_PIN 35
#define SERVO_PIN 13

// 🔴 LED PINS
#define GREEN_LED 25
#define BLUE_LED 26
#define RED_LED 33

int mq2_threshold = 1000;
int mq135_threshold = 2760;

Servo myServo;

int servoPos = 0;
int servoDir = 1;

bool servoAttached = true;

// ✅ NEW: fire trigger control
bool fireSent = false;

void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);

  // LED setup
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  myServo.attach(SERVO_PIN, 500, 2500);
  myServo.write(0);
}

void loop() {

  int flame = digitalRead(IR_PIN);

  // 🔥 FIRE (IR LOW)
  if (flame == LOW) {

    Serial.println("\n🔥 FIRE DETECTED (IR)");

    // ✅ SEND ONLY ONCE
    if (!fireSent) {
      Serial.println("FIRE");
      fireSent = true;
    }

    // STOP SERVO
    if (servoAttached) {
      myServo.detach();
      servoAttached = false;
    }

    int mq2 = analogRead(MQ2_PIN);
    int mq135 = analogRead(MQ135_PIN);

    Serial.print("MQ2: ");
    Serial.print(mq2);
    Serial.print(" | MQ135: ");
    Serial.println(mq135);

    // 🔥 CLASSIFICATION + LED LOGIC

    if (mq2 > mq2_threshold && mq135 <= mq135_threshold) {
      Serial.println("⚠️ GAS / LPG FIRE");

      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, LOW);

    } 
    else if (mq135 > mq135_threshold && mq2 <= mq2_threshold) {
      Serial.println("⚠️ WOOD / SOLID FIRE");

      digitalWrite(RED_LED, HIGH);
      digitalWrite(BLUE_LED, LOW);
      digitalWrite(GREEN_LED, LOW);

    } 
    else if (mq2 > mq2_threshold && mq135 > mq135_threshold) {
      Serial.println("⚠️ MIXED FIRE");

      digitalWrite(RED_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);

    } 
    else {
      Serial.println("🔥 Flame but low gas signature");

      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      digitalWrite(RED_LED, LOW);
    }

    Serial.println("----------------------");

    delay(200);
    return;
  }

  // ✅ NO FIRE → RESET FIRE FLAG
  fireSent = false;

  // ✅ SAFE MODE LED
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(RED_LED, LOW);

  if (!servoAttached) {
    Serial.println("\n✅ NO FIRE → SERVO RESUME");

    myServo.attach(SERVO_PIN, 500, 2500);
    myServo.write(servoPos);

    servoAttached = true;
  }

  // 🔁 SCANNING
  servoPos += servoDir * 2;

  if (servoPos >= 180) { servoPos = 180; servoDir = -1; }
  if (servoPos <= 0)   { servoPos = 0;   servoDir = 1; }

  myServo.write(servoPos);

  Serial.print("Scanning: ");
  Serial.println(servoPos);

  delay(20);
}