#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "BluetoothSerial.h"

#define PIN_ENA   2
#define PIN_IN1   4
#define PIN_IN2   0
#define PIN_POT   34
#define PWM_CHANNEL   0
#define PWM_FREQ      1000
#define PWM_RES       8
#define NUM_SAMPLES   10

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define BT_TIMEOUT_MS 3000

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
BluetoothSerial SerialBT;

int btTargetPWM = -1;          // -1 means no active BT command
unsigned long lastBTCommandTime = 0;
bool btActive = false;

int readPotSmoothed() {
  long sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += analogRead(PIN_POT);
    delay(1);
  }
  return sum / NUM_SAMPLES;
}

// Parses commands like "S150" -> sets btTargetPWM = 150
void handleBTInput() {
  static String buf = "";
  while (SerialBT.available()) {
    char c = SerialBT.read();
    if (c == '\n' || c == '\r') {
      if (buf.length() > 0) {
        if (buf[0] == 'S' || buf[0] == 's') {
          int val = buf.substring(1).toInt();
          val = constrain(val, 0, 255);
          btTargetPWM = val;
          lastBTCommandTime = millis();
          btActive = true;
          SerialBT.print("OK PWM=");
          SerialBT.println(val);
        }
        buf = "";
      }
    } else {
      buf += c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_ENA, PWM_CHANNEL);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  SerialBT.begin("ESP32_MotorCtrl");  // Bluetooth device name
  Serial.println("Motor controller ready. BT: ESP32_MotorCtrl");
}

void loop() {
  handleBTInput();

  // Failsafe: revert to pot if BT goes silent
  if (btActive && (millis() - lastBTCommandTime > BT_TIMEOUT_MS)) {
    btActive = false;
    btTargetPWM = -1;
  }

  int adcVal = readPotSmoothed();
  int potPWM = map(adcVal, 0, 2600, 0, 255);
  potPWM = constrain(potPWM, 0, 255);

  int pwmVal = btActive ? btTargetPWM : potPWM;
  ledcWrite(PWM_CHANNEL, pwmVal);

  Serial.print("ADC: ");
  Serial.print(adcVal);
  Serial.print(" | PWM: ");
  Serial.print(pwmVal);
  Serial.println(btActive ? " | Source: BT" : " | Source: POT");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println("MOTOR CTRL");
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print("ADC: ");
  display.println(adcVal);
  display.setCursor(0, 36);
  display.print("PWM: ");
  display.println(pwmVal);
  display.setCursor(0, 48);
  int pct = map(pwmVal, 0, 255, 0, 100);
  display.print("Spd:");
  display.print(pct);
  display.print("% ");
  display.println(btActive ? "[BT]" : "[POT]");
  display.display();

  delay(50);
}