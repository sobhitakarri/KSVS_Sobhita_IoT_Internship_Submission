# IoT-Based Patient Monitoring System
**ESP32 · FreeRTOS · MQTT · Adafruit IO Dashboard · Wokwi Simulation**

**Author:** KSVS Sobhita  
**Platform:** Wokwi Simulator + Adafruit IO  
**Microcontroller:** ESP32 Dual-Core (240 MHz)  
**Simulation Link:** [Wokwi Project #465449721853605889](https://wokwi.com/projects/465449721853605889)  

---

## 📌 Table of Contents
- [Executive Overview](#-executive-overview)
- [Key Features](#-key-features)
- [System Architecture](#-system-architecture)
- [Hardware & Pin Mapping](#-hardware--pin-mapping)
- [FreeRTOS Multitasking Design](#-freertos-multitasking-design)
- [MQTT Feeds & Remote Telemetry](#-mqtt-feeds--remote-telemetry)
- [Adafruit IO Dashboards](#-adafruit-io-dashboards)
- [Fault Injection & Alert Mechanisms](#-fault-injection--alert-mechanisms)
- [Offline Failsafe & Network Resilience](#-offline-failsafe--network-resilience)
- [Project Directory Structure](#-project-directory-structure)
- [Getting Started & Simulation Setup](#-getting-started--simulation-setup)
- [Mandatory Documents](#-mandatory-documents)

---

## 🏥 Executive Overview

Modern healthcare facilities demand continuous, real-time patient vital sign and ambient environmental monitoring to safeguard patients and ensure rapid emergency responses. This project delivers an advanced, end-to-end IoT Patient Monitoring & Actuation System built on the Espressif ESP32 dual-core microcontroller running FreeRTOS and communicating with the Adafruit IO cloud platform over MQTT.

The system tracks vital signs (Heart Rate, SpO2, Body Temperature) and environmental parameters (Ambient Temperature, Room Oxygen concentration, Air Quality Index) while enabling bi-directional remote control for IV medication dosage adjustments, motorized bed elevation, and dynamic telemetry rate throttling. A dedicated offline failsafe guarantees continuous local monitoring, audio alarms, and OLED telemetry even during network blackouts.

---

## ✨ Key Features

1. **Dual-Core FreeRTOS Concurrency:** Hardware acquisition runs on Core 1 while network I/O, OLED rendering, and alarm evaluation execute on Core 0 with zero blocking.
2. **Comprehensive Vital Signs Tracking:** Continuous monitoring of Heart Rate (BPM), Blood Oxygen Saturation (SpO2 %), and Body Temperature (°C).
3. **Hospital Environment Surveillance:** Real-time tracking of Room Temperature (°C), Room Oxygen (%), and Air Quality Index (AQI).
4. **Bi-Directional Remote Actuation:**
   - **IV Dosage Rate:** Remotely set infusion rates (0–100 mg) with automatic local hazard alerts (> 80 mg).
   - **Bed Elevation Angle:** Remote servo motor position control (0–180°) supporting clinical positions (CPR flat, Fowler's, Respiratory support).
   - **Dynamic Sampling Rate:** Remote cloud publish interval throttling (5 s to 60 s) to optimize bandwidth and quota.
5. **Local Real-Time OLED Dashboard:** 128×64 SSD1306 display provides instant on-site status, vitals, environmental metrics, and connection diagnostics.
6. **Active Fault Injection Bank:** DIP switch bank to inject clinical anomalies (Fever, Tachycardia, Hypoxia) and network outages for verification.
7. **Resilient Offline Failsafe:** Autonomous transition to offline monitoring with dedicated audible alarms and exponential backoff auto-reconnection.

---

## 🏗️ System Architecture

```
                      +------------------------------------------+
                      |         Adafruit IO Cloud Platform       |
                      |  - Patient Monitoring Dashboard (Vitals) |
                      |  - Facility Management Dashboard (Env)   |
                      +--------------------+---------------------+
                                           |  MQTT Protocol (Port 1883)
                                           v
+------------------------------------------------------------------------------------+
|                                    ESP32 SoC                                       |
|                                                                                    |
|  +-------------------------------------+   +------------------------------------+  |
|  |             CORE 1                  |   |              CORE 0                |  |
|  |  (Hardware & Sensor Acquisition)    |   |  (Processing, Display & Network)   |  |
|  |                                     |   |                                    |  |
|  |  [sensorTask (Prio 1, 1000ms)]      |   |  [processingTask (Prio 2, 250ms)]  |  |
|  |  - DS18B20 Temp Probe               |   |  - Read Queues (xQueuePeek)        |  |
|  |  - HR & SpO2 Generator              |   |  - SSD1306 OLED (I2C)              |  |
|  |  - DIP Switch Fault Monitoring      |   |  - Remote Subs (Dosage/Bed/Rate)   |  |
|  |                 |                   |   |  - Multi-Zone Alarm Evaluation     |  |
|  |                 v (xQueueOverwrite) |   |  - Servo PWM Control (GPIO 18)     |  |
|  |         [FreeRTOS Queues]           |   |  - MQTT Publish (Dynamic Rate)     |  |
|  |  (hrQ, spo2Q, roomTQ, bodyTQ, etc.) |-->|  - Offline Detection & Reconnect   |  |
|  |                 ^                   |   +------------------------------------+  |
|  |  [advSensorTask (Prio 1, 2000ms)]   |                                           |
|  |  - Room O2 & AQI Potentiometers     |                                           |
|  |  - Body Temp Fever Simulation       |                                           |
|  +-------------------------------------+                                           |
+------------------------------------------------------------------------------------+
```

---

## 🔌 Hardware & Pin Mapping

| Component | Function / Role | Interface Type | ESP32 GPIO |
| :--- | :--- | :--- | :--- |
| **DS18B20 Sensor** | Ambient Room Temperature | 1-Wire Digital | `GPIO 14` |
| **PIR Sensor** | Patient Motion Detector | Digital Input | `GPIO 12` |
| **O2 Potentiometer** | Room Oxygen Concentration (15–25%) | 12-bit ADC Input | `GPIO 35` |
| **AQI Potentiometer** | Air Quality Index (0–500) | 12-bit ADC Input | `GPIO 39` |
| **SSD1306 OLED** | 128×64 Local Graphic Dashboard | I2C (SDA / SCL) | `GPIO 21` / `GPIO 22` |
| **Servo Motor** | Bed Elevation Mechanism (0–180°) | PWM Output | `GPIO 18` |
| **IV Warning LED** | High Dosage Warning Indicator (>80mg) | Digital Output | `GPIO 27` |
| **Buzzer 1** | Body Temperature Alarm (Fever) | Digital Output | `GPIO 4` |
| **Buzzer 2** | Heart Rate / BP Alarm (Tachycardia) | Digital Output | `GPIO 16` |
| **Buzzer 3** | Blood Oxygen / Respiratory Alarm (Hypoxia)| Digital Output | `GPIO 17` |
| **Buzzer 4** | Room Climate Alarm (Overheat) | Digital Output | `GPIO 5` |
| **Offline Buzzer** | Network Loss / Failsafe Alarm | Digital Output | `GPIO 19` |
| **DIP 1 (Kill Switch)**| Hardware Simulated Network Blackout | Digital In (Pull-up)| `GPIO 13` |
| **DIP 8 (SW_TEMP)** | Fever Fault Injection | Digital In (Pull-up)| `GPIO 32` |
| **DIP 7 (SW_HR)** | Tachycardia Fault Injection | Digital In (Pull-up)| `GPIO 33` |
| **DIP 6 (SW_SPO2)** | Hypoxia Fault Injection | Digital In (Pull-up)| `GPIO 25` |
| **DIP 5 (SW_MOTION)** | Motion Bypass Fault Injection | Digital In (Pull-up)| `GPIO 26` |

---

## ⚙️ FreeRTOS Multitasking Design

The firmware leverages FreeRTOS dual-core task scheduling and mailbox queues:

1. **`sensorTask` (Core 1, Priority 1, 1000 ms):**
   - Polls DS18B20 digital temperature probe via OneWire.
   - Generates simulated Heart Rate (65–85 BPM normal, 110–135 BPM during tachycardia) and SpO2 (95–100% normal, 82–88% during hypoxia).
   - Overwrites `hrQueue`, `spo2Queue`, and `roomTempQueue` using `xQueueOverwrite()`.

2. **`advancedSensorTask` (Core 1, Priority 1, 2000 ms):**
   - Samples 12-bit analog potentiometers for Room O2 and AQI.
   - Computes Body Temperature (36.5–37.1°C normal, 38.5–39.4°C during fever).
   - Overwrites `roomO2Queue`, `aqiQueue`, and `bodyTempQueue`.

3. **`processingTask` (Core 0, Priority 2, 250 ms):**
   - Highest priority task ensuring deterministic 4 Hz UI updates and rapid alarm response.
   - Peeks latest data from all 6 queues with zero blocking.
   - Evaluates multi-zone safety thresholds and triggers audio/visual alarms.
   - Reads incoming Adafruit IO commands (`dosage`, `bed-elevation`, `publish-rate`).
   - Executes dynamic non-blocking MQTT telemetry publishing.

---

## 📡 MQTT Feeds & Remote Telemetry

All feeds are hosted under the Adafruit IO namespace:

| Feed Name | Direction | Data Type / Range | Description |
| :--- | :--- | :--- | :--- |
| `heart-rate-monitor` | Outbound (Pub) | Integer (`60–150 BPM`) | Real-time Heart Rate |
| `spo2` | Outbound (Pub) | Integer (`82–100 %`) | Blood Oxygen Saturation |
| `body-temp` | Outbound (Pub) | Float (`36.5–39.5 °C`) | Patient Body Temperature |
| `room-temp` | Outbound (Pub) | Float (`10–50 °C`) | Ambient Ward Temperature |
| `room-oxygen` | Outbound (Pub) | Integer (`15–25 %`) | Room Oxygen Concentration |
| `air-quality` | Outbound (Pub) | Integer (`0–500 AQI`) | Air Quality Index |
| `motion-detector` | Outbound (Pub) | Boolean (`0 / 1`) | Bed Movement / Activity |
| `flag` | Outbound (Pub) | String (Status text) | Diagnostic Status / Alarm State |
| `dosage` | Inbound (Sub) | Integer (`0–100 mg`) | Remote IV Infusion Rate Control |
| `bed-elevation` | Inbound (Sub) | Integer (`0–180 °`) | Remote Bed Angle Servo Command |
| `publish-rate` | Inbound (Sub) | Integer (`5–60 s`) | Remote Telemetry Interval Adjustment |

---

## 📊 Adafruit IO Dashboards

The system features two dedicated dashboards accessible online:

- **Facility Management View:** [https://io.adafruit.com/Sobhita/dashboards/facility-management-view](https://io.adafruit.com/Sobhita/dashboards/facility-management-view)  
  *Designed for hospital estates & facility engineers to monitor Room Temp, Room O2, and AQI with visual color-coded alert zones.*
- **Medical Staff / Patient Monitoring View:** [https://io.adafruit.com/Sobhita/dashboards/medical-staff-view](https://io.adafruit.com/Sobhita/dashboards/medical-staff-view)  
  *Designed for clinicians and nursing staff to view live vitals, IV dosage sliders, bed incline controls, and dynamic sampling rate inputs.*

---

## 🚨 Fault Injection & Alert Thresholds

| Clinical Condition | Trigger Mechanism | Alarm / Actuator Reaction |
| :--- | :--- | :--- |
| **High IV Dose (> 80 mg)** | Cloud Slider Command | GPIO 27 LED ON, OLED & Cloud Warning Flag |
| **Fever (> 37.8 °C)** | DIP Switch 8 / Sensor | Buzzer 1 Activated, Status "Vitals: Fever!" |
| **Tachycardia (> 100 BPM)** | DIP Switch 7 / Sensor | Buzzer 2 Activated, Status "Vitals: High HR!" |
| **Hypoxia (< 90 % SpO2)** | DIP Switch 6 / Sensor | Buzzer 3 Activated, Status "Vitals: Low SpO2!" |
| **Ward Overheating (> 28 °C)** | DS18B20 / Pot | Buzzer 4 Activated, Status "Env: Room Too Hot!" |
| **O2 Hazard (< 19% or > 23%)**| Potentiometer (ADC) | Status "Env: O2 Hazard!" |
| **Hazardous AQI (> 100)** | Potentiometer (ADC) | Status "Env: Poor Air Quality!" |

---

## 🛡️ Offline Failsafe & Network Resilience

When network loss occurs (or when triggered by DIP Switch 1):
1. **Instant Detection:** Evaluates `WiFi.status()`, `mqtt.connected()`, and Kill Switch status every 250 ms.
2. **Audio/Visual Alert:** OLED displays `LOGGING OFFLINE` and the dedicated Offline Buzzer (GPIO 19) pulses every 250 ms.
3. **Safe Disconnect:** MQTT sockets are cleanly closed to prevent memory leaks or frozen connections.
4. **Autonomous Operation:** Sensor acquisition, local alarms, OLED vitals display, and bed elevation continue uninterrupted.
5. **Staged Reconnection:** Automatic non-blocking retry every 5,000 ms reconnecting WiFi first, then MQTT.

---

## 📂 Project Directory Structure

```
KSVS_Sobhita_IoT_Internship_Submission/
│
├── src/                               # Firmware & Hardware Simulation
│   ├── sketch.ino                     # Main ESP32 FreeRTOS & MQTT Firmware
│   ├── diagram.json                   # Complete Wokwi Hardware Circuit Schematic
│   ├── libraries.txt                  # Third-party Arduino/ESP32 Library Manifest
│   └── wokwi-project.txt              # Wokwi Web Simulator Project Identifier
│
├── docs/                              # Project Documentation & Reports
│   ├── KSVS_Sobhita_Report.pdf        # Full Technical Report & Evaluation
│   ├── Dashboard_Links.txt            # Live Adafruit IO Dashboard URLs
│   └── wokwi-project.txt              # Cloud Simulation Link Reference
│
├── media/                             # Video Demonstration & Assets
│   └── KSVS_Sobhita_Demo.mp4          # Complete Video Demonstration
│
├── README.md                          # Main Comprehensive Project Documentation
├── BUG_LOG.md                         # Detailed Bug & Issue Resolution Log
├── ASSUMPTIONS.md                     # Engineering Assumptions & Design Boundaries
├── SELF_ATTESTATION.md                # Submission & Self-Attestation Checklist
└── .gitignore                         # Git Ignore Configuration
```

---

## 🚀 Getting Started & Simulation Setup

### Running in Wokwi Simulator
1. Open the project directly in your browser: [Wokwi Simulation Link](https://wokwi.com/projects/465449721853605889)
2. Click the green **Start Simulation** button.
3. Observe the ESP32 connecting to `Wokwi-GUEST` WiFi and Adafruit IO MQTT.
4. Use DIP switches to test fault conditions:
   - Toggle **DIP 8** for Fever.
   - Toggle **DIP 7** for Tachycardia.
   - Toggle **DIP 6** for Hypoxia.
   - Toggle **DIP 1** for Network Disconnect.
5. Adjust potentiometers to modify Room Oxygen and AQI.
6. Open the [Adafruit IO Medical Staff Dashboard](https://io.adafruit.com/Sobhita/dashboards/medical-staff-view) to send remote slider commands for dosage, bed elevation, and publish interval.

### Building with Arduino IDE / PlatformIO
1. Install ESP32 Board Support in Arduino IDE.
2. Install the required libraries from `src/libraries.txt`:
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
   - `Adafruit MQTT Library`
   - `OneWire`
   - `DallasTemperature`
   - `ESP32Servo`
3. Open `src/sketch.ino`, configure your WiFi & Adafruit IO credentials, and upload to an ESP32 development board.

---

## 📋 Mandatory Documents

- 📄 **[Bug Log](BUG_LOG.md):** Complete catalog of technical issues encountered during development and their resolutions.
- 📄 **[Assumptions Document](ASSUMPTIONS.md):** Detailed analysis of hardware, sensor simulation, and communication boundaries.
- 📄 **[Self-Attestation Checklist](SELF_ATTESTATION.md):** Verification checklist ensuring all submission criteria are fulfilled.
- 📄 **[Technical Report](docs/KSVS_Sobhita_Report.pdf):** Comprehensive technical report with circuit schematics and task graphs.
