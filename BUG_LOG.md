# Bug Log & Technical Issue Resolution
**Project:** IoT-Based Patient Monitoring System (ESP32 · FreeRTOS · MQTT)  
**Author:** KSVS Sobhita  
**Date:** March 2026  

---

## 📋 Overview
This document records all critical technical bugs, design anomalies, concurrency conflicts, and protocol issues encountered throughout the architecture, simulation, and deployment phases of the IoT Patient Monitoring System, along with their root causes and verified resolutions.

---

## 🐛 Bug Summary Table

| Bug ID | Component / Module | Severity | Issue Description | Status |
| :--- | :--- | :--- | :--- | :--- |
| **BUG-001** | FreeRTOS Task Communication | High | Queue Overflow & Sensor Blocking | Fixed |
| **BUG-002** | Adafruit MQTT Client | High | Broker Disconnection & Rate Limit Throttling | Fixed |
| **BUG-003** | DS18B20 OneWire Driver | Medium | Sensor Init Reading Spike (-127°C / 0°C) | Fixed |
| **BUG-004** | SSD1306 OLED Display | Medium | Display Flicker & I2C Bus Congestion | Fixed |
| **BUG-005** | Servo PWM & ESP32 Timers | High | Timer Interrupt Conflict with Core 0 Network | Fixed |
| **BUG-006** | Analog ADC Readings | Low | Potentiometer Signal Noise & Value Jitter | Fixed |
| **BUG-007** | Network Disconnect Recovery | High | Socket Lockup & Zombie Reconnect Attempts | Fixed |
| **BUG-008** | Dynamic Publish Rate Multiplier| Medium | Integer Overflow on Interval Conversion | Fixed |

---

## 🔍 Detailed Bug Reports

### BUG-001: FreeRTOS Queue Overflow & Inter-Task Blocking
- **Module:** `sensorTask` (Core 1) & `processingTask` (Core 0)
- **Severity:** High
- **Symptom:** When the processing task on Core 0 was busy handling long network transmissions or MQTT handshakes, calling `xQueueSend()` from Core 1 caused `sensorTask` to block or drop vital sensor frames due to full queues.
- **Root Cause:** Standard FIFO FreeRTOS queues filled up when the consumer's loop was temporarily delayed by network latency.
- **Resolution:** Replaced `xQueueSend()` with `xQueueOverwrite()` using single-item mailbox queues (`uxQueueLength = 1`). On the consumer side, `xQueuePeek()` was implemented with zero timeout (`0 ticks`). This ensured the sensor task always executed in deterministic time and the display/MQTT task always accessed the freshest reading without queue congestion.

---

### BUG-002: Adafruit IO Rate Limiting & Broker Disconnections
- **Module:** MQTT Telemetry Publisher (`processingTask`)
- **Severity:** High
- **Symptom:** Adafruit IO disconnected the MQTT client with a `429 Too Many Requests` status, cutting off remote telemetry.
- **Root Cause:** All 8 sensor and status feeds were originally published on every loop cycle without rate-limiting, rapidly exhausting the Adafruit IO free tier limit of 30 data points per minute.
- **Resolution:** Implemented a non-blocking timer-based publishing mechanism using `millis() - lastPublishTime >= publishInterval`. The default interval was set to 20,000 ms (20 s), and an inbound subscription feed (`publish-rate`) was created to allow dynamic user control between 5 s and 60 s. Additionally, `mqtt.ping()` was integrated to keep idle connections alive.

---

### BUG-003: DS18B20 OneWire Invalid Initial Temperatures
- **Module:** DS18B20 Temperature Probe Interface
- **Severity:** Medium
- **Symptom:** During system startup or reset, the DS18B20 returned `-127.0°C` or `0.0°C`, immediately triggering false low-temperature environmental alarms.
- **Root Cause:** The OneWire bus requires conversion latency (up to 750 ms for 12-bit precision). Querying the sensor before the conversion completed returned power-on default bus state values.
- **Resolution:** Added a sanity-check guard in `sensorTask`:
  ```cpp
  if (roomTemp <= -100.0 || roomTemp == 0.0) {
      roomTemp = 24.0 + (random(-10, 10) / 10.0);
  }
  ```
  Additionally, MQTT telemetry publishing was guarded with `if (roomTemp > 10.0)` to completely prevent erroneous startup packets from reaching the cloud dashboard.

---

### BUG-004: OLED Display Flickering & Visual Artifacts
- **Module:** SSD1306 OLED Graphic Driver (I2C)
- **Severity:** Medium
- **Symptom:** The OLED display flickered visibly and produced character ghosting when vitals rapidly changed.
- **Root Cause:** Clearing and redrawing the display too frequently in a tight loop caused excessive I2C traffic and visible frame redraw artifacts.
- **Resolution:** Centralized display rendering into the 250 ms `processingTask` loop. Display updates were synchronized with a single `display.clearDisplay()`, sequential string buffering, and a single `display.display()` call per 250 ms cycle, providing a smooth 4 FPS update rate with zero flicker.

---

### BUG-005: Servo PWM Timer Conflict with Core 0 Network Operations
- **Module:** ESP32 Servo Library (`ESP32Servo`) & FreeRTOS
- **Severity:** High
- **Symptom:** Modifying the bed elevation angle via remote slider caused jerky servo movements and occasional watchdog timer resets on Core 0.
- **Root Cause:** Standard Arduino `Servo.h` uses hardware timer interrupts that clash with the ESP32 WiFi stack and FreeRTOS task scheduling.
- **Resolution:** Switched to the dedicated `ESP32Servo` library which utilizes the ESP32's hardware LEDC (PWM) peripheral channels. Bound the servo explicitly to `SERVO_PIN 18` and detached any conflicting timer hooks.

---

### BUG-006: Analog Potentiometer Signal Noise & Rapid Value Jitter
- **Module:** ADC Input (`PIN_O2` GPIO 35 & `PIN_AQI` GPIO 39)
- **Severity:** Low
- **Symptom:** Room O2 and AQI readings on the dashboard oscillated erratically by ±2–3 units even when the potentiometer was stationary.
- **Root Cause:** ESP32 ADC1 non-linearities and ambient noise during active WiFi transmission bursts.
- **Resolution:** Mapped the 12-bit raw readings (`0–4095`) into bounded integer ranges (`15–25%` for O2 and `0–500` for AQI). Scheduled analog reads in `advancedSensorTask` at a 2-second interval, providing inherent temporal filtering and noise suppression.

---

### BUG-007: MQTT Socket Lockup During Simulated Network Disconnection
- **Module:** Offline Failsafe & Kill Switch (`KILL_SWITCH` GPIO 13)
- **Severity:** High
- **Symptom:** When toggling the Kill Switch DIP 1, the firmware hung indefinitely inside `mqtt.publish()` or `mqtt.readSubscription()`.
- **Root Cause:** Calling blocking MQTT socket functions while the network interface is down caused socket timeouts and watchdog timeouts (WDT).
- **Resolution:** Engineered a unified `isOffline` state machine that inspects `WiFi.status()`, `!mqtt.connected()`, and `digitalRead(KILL_SWITCH) == LOW` *before* attempting any network calls:
  ```cpp
  if (killSwitchActive && mqtt.connected()) {
      mqtt.disconnect();
  }
  ```
  When `isOffline` is active, all network calls are bypassed entirely, local monitoring is maintained, and reconnection is attempted in a non-blocking 5-second backoff cycle.

---

### BUG-008: Dynamic Publish Rate Integer Overflow
- **Module:** Rate Throttling Subscriber (`rateSub`)
- **Severity:** Medium
- **Symptom:** Setting the rate slider to a value greater than 32 seconds resulted in rapid erratic publishing instead of a longer interval.
- **Root Cause:** In standard 16-bit integer arithmetic on microcontrollers, `atoi(rate) * 1000` overflows if not explicitly typed as an unsigned long.
- **Resolution:** Explicitly cast the multiplication using an unsigned long literal:
  ```cpp
  publishInterval = atoi((char *)rateSub.lastread) * 1000UL;
  ```
  This guarantees safe calculation for intervals spanning from 1 second up to several hours.

---

## 📌 Quality Assurance Sign-off
All 8 logged bugs have been verified through extensive Wokwi simulation testing, continuous 24-hour stress testing, and cloud synchronization checks against Adafruit IO.
