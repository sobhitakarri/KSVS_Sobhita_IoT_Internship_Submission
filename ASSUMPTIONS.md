# Engineering Assumptions & Design Boundaries
**Project:** IoT-Based Patient Monitoring System (ESP32 · FreeRTOS · MQTT)  
**Author:** KSVS Sobhita  
**Date:** March 2026  

---

## 📌 Introduction
This document formalizes all architectural, physical, clinical, communication, and simulation assumptions adopted during the development of the IoT-based patient monitoring system. These assumptions define the operating parameters, safety boundaries, and target environment under which the system operates.

---

## 1. Hardware & Physical Environment Assumptions

### 1.1 Microcontroller Capabilities
- **Platform:** Espressif ESP32-WROOM-32 (240 MHz dual-core Xtensa LX6 processor).
- **Core Allocation:**
  - **Core 1 (Application / Hardware Core):** Dedicated exclusively to hard real-time sensor polling, ADC sampling, and DIP-switch state acquisition.
  - **Core 0 (Protocol / UI Core):** Dedicated to WiFi stack processing, MQTT network transactions, OLED I2C rendering, and audio/visual alarm generation.
- **Power Supply:** Stable 5V USB/regulator supply with sufficient current capacity (minimum 500 mA) to drive the ESP32 radio bursts, OLED display, 5 buzzers, and the servo motor concurrently without brownout resets.

### 1.2 Sensor Emulation & Physical Models
- **Room Temperature (DS18B20):** Assumed to operate over 1-Wire protocol on GPIO 14. In real deployment, probe represents patient room/ward temperature. Normal operating range: 20°C–26°C. Warning threshold: > 28°C.
- **Patient Motion (PIR Sensor):** Modeled as a binary digital input (GPIO 12) simulating patient movement detection or bed exit.
- **Room Oxygen Concentration (Potentiometer on GPIO 35):** 12-bit ADC mapped linearly from 0–4095 to 15%–25% O2. Normal atmospheric concentration is ~20.9%. Clinical hazard bounds: < 19% (hypoxic hazard) or > 23% (combustion/hyperoxic hazard).
- **Air Quality Index (Potentiometer on GPIO 39):** 12-bit ADC mapped linearly from 0–4095 to 0–500 AQI. Normal indoor hospital air quality is assumed < 50 AQI; alerts trigger when AQI > 100.
- **Patient Body Temperature:** Synthesized within normal physiological boundaries (36.5°C–37.1°C) with fever escalation (38.5°C–39.4°C) triggered via DIP switch 8.
- **Heart Rate & SpO2:** Synthesized with physiological variance (Normal HR: 65–85 BPM, Tachycardia: 110–135 BPM; Normal SpO2: 95–100%, Hypoxia: 82–88%).

---

## 2. Clinical & Medical Logic Assumptions

### 2.1 Patient Vital Sign Thresholds
- **Body Temperature:** Fever threshold is established at `> 37.8°C` based on standard clinical definitions of pyrexia.
- **Heart Rate:** Normal resting adult range is 60–100 BPM. Tachycardia alert is triggered when `HR > 100 BPM`.
- **Blood Oxygen Saturation (SpO2):** Normal healthy range is 95–100%. Hypoxemia alert is triggered when `SpO2 < 90%`.

### 2.2 Remote Intravenous (IV) Dosage Adjustment
- **Dosage Range:** 0 to 100 mg/hour (simulated infusion pump rate).
- **Hazard Upper Bound:** 80 mg/hour is assumed as the maximum safe clinical limit for the simulated medication. Any command exceeding 80 mg immediately triggers a local visual alert (IV Warning LED on GPIO 27) and posts a high-priority warning flag to the cloud.

### 2.3 Bed Elevation Angle Mapping
- **Angle Range:** 0° to 180° mapped 1:1 with the servo motor horn position.
- **Clinical Position Mapping:**
  - `0°`: Fully horizontal / flat (used for CPR, post-surgical recovery, or transport).
  - `30°–45°`: Low/Semi-Fowler's position (used for assisted respiratory recovery and eating).
  - `90°`: Upright Fowler's position (used for active patient engagement and respiratory relief).
  - `180°`: Maximum elevation / special care position.

---

## 3. Networking & Cloud Infrastructure Assumptions

### 3.1 Network Protocol & Broker
- **Protocol:** MQTT (Message Queuing Telemetry Transport) v3.1.1 over TCP port 1883.
- **Broker Platform:** Adafruit IO (`io.adafruit.com`).
- **Connection Security:** Uses username and AIO API Key authentication. (In a full hospital production deployment, TLS/MQTTS on port 8883 with certificate validation would be utilized).

### 3.2 Dynamic Telemetry Throttling & Quotas
- **Default Telemetry Interval:** 20 seconds (20,000 ms) per full telemetry batch.
- **Rate Limit Window:** Adafruit IO free accounts permit up to 30 data points per minute. The default interval publishes 8 feeds every 20 seconds (24 data points/minute), staying strictly within free-tier quota limits.
- **Dynamic Range:** Clinicians can adjust the rate between 5 seconds (for critical emergencies) and 60 seconds (for routine ward monitoring).

### 3.3 Offline Resilience & Failsafe
- **Zero Data Loss on Sensor Side:** Sensor readings continue to be updated in FreeRTOS single-slot queues regardless of network connectivity status.
- **Local Autonomy:** Audio alarms, visual LED warnings, OLED status updates, and bed elevation remain fully operational during complete WiFi/broker dropouts.
- **Automatic Staged Recovery:** Reconnection attempts occur in 5,000 ms intervals without blocking real-time loops.

---

## 4. Simulation Environment Assumptions (Wokwi)

- **Simulator Clock:** Runs in real-time or scaled simulated time based on host browser performance.
- **WiFi Emulation:** Connects via simulated `Wokwi-GUEST` access point with open security.
- **Fault Injection DIP Switches:** Modeled with active-low logic (`INPUT_PULLUP`):
  - Switch ON (Closed to GND) = Logic `LOW` (Fault active).
  - Switch OFF (Open) = Logic `HIGH` (Normal operation).
