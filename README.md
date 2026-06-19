# ESP32 Open-Loop DC Motor Speed Controller

A PWM-based DC motor speed controller built on the ESP32, featuring potentiometer input, OLED telemetry, and two-way Bluetooth control with automatic failsafe.

> **Note on scope:** This project was originally planned as a closed-loop PID controller with encoder feedback. The motor ordered (CQRobot 70:1 12V DC gear motor) does not include an encoder, so the scope was revised to open-loop control. Closed-loop PID may be revisited if an encoder is added later.

## Features

- PWM motor speed control via L298N H-bridge
- Analog speed control via potentiometer
- Real-time OLED display (ADC value, PWM value, speed %, active control source)
- Two-way Bluetooth control (classic SPP) — set speed remotely from a phone
- Automatic failsafe: reverts to potentiometer control if Bluetooth goes idle for 3 seconds

## Hardware

| Component | Part |
|---|---|
| MCU | HiLetgo ESP32 WROOM-32 (38-pin, USB-C, CP2102) |
| Motor | CQRobot 70:1 12V DC gear motor (no encoder) |
| Motor Driver | WWZMDiB L298N H-bridge |
| Display | ELEGOO SSD1306 0.96" 128×64 I2C OLED |
| Speed Input | TWTADE 10kΩ potentiometer |
| Power | 12V 2A adapter (motor), USB (ESP32 logic) |

## Wiring

| Signal | ESP32 Pin |
|---|---|
| ENA (PWM) | GPIO2 |
| IN1 | GPIO4 |
| IN2 | GPIO0 |
| Potentiometer wiper | GPIO34 |
| OLED SDA | GPIO21 |
| OLED SCL | GPIO22 |

**Power:**
- 12V adapter → L298N 12V input (motor power only — do **not** route through ESP32 or a low-current breadboard power module)
- ESP32 powered separately via USB
- **Common ground required** between the 12V supply, L298N, and ESP32

**L298N jumpers:**
- 5V enable jumper: **ON**
- ENA / ENB jumpers: **removed** (PWM is driven directly from ESP32 GPIO2)

**OLED I2C address:** `0x3C` (verify with an I2C scanner sketch if using a different module)

## Firmware

- **IDE:** Arduino IDE 2.x
- **Language:** C++
- **Libraries:**
  - `Adafruit_GFX`
  - `Adafruit_SSD1306`
  - `BluetoothSerial` (built into ESP32 board package)

### Key implementation notes

- The ESP32's ADC does not reach its theoretical 12-bit max (4095) in practice on this hardware — it tops out around **2600**. The PWM mapping is calibrated to this real ceiling:
  ```cpp
  int pwmVal = map(adcVal, 0, 2600, 0, 255);
  pwmVal = constrain(pwmVal, 0, 255); // clamps in case ADC exceeds 2600
  ```
- Potentiometer readings are averaged over 10 samples to reduce noise.

## Bluetooth Control

- **Device name:** `ESP32_MotorCtrl`
- **Protocol:** classic Bluetooth SPP (not BLE — **not compatible with iOS**, which doesn't support classic SPP for third-party apps)
- **Command format:** `S<value>` where value is `0`–`255`, e.g. `S200`
- **Failsafe:** if no command is received for 3 seconds, control automatically reverts to the potentiometer
- **Recommended app:** [Serial Bluetooth Terminal](https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal) (Android)

### Testing Bluetooth control

1. Pair phone with `ESP32_MotorCtrl` in Bluetooth settings
2. Open Serial Bluetooth Terminal, connect to the device
3. Send `S200` (with newline) — motor speed jumps to that PWM value, OLED shows `[BT]`
4. Stop sending — after 3 seconds, control reverts to the potentiometer, OLED shows `[POT]`

## Future Work

- Add a rotary encoder to the motor and revisit closed-loop PID speed control
- BLE rewrite for iOS compatibility
- Wi-Fi web dashboard as an alternative to Bluetooth

## License

MIT
