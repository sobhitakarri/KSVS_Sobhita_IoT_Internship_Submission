# Self-Attestation Checklist & Verification
**Project:** IoT-Based Patient Monitoring System (ESP32 · FreeRTOS · MQTT)  
**Author:** KSVS Sobhita  
**Submission Date:** March 2026  

---

## 📋 Student & Submission Details

- **Student Name:** KSVS Sobhita
- **Project Title:** IoT-Based Patient Monitoring System with Dual-Core FreeRTOS & MQTT
- **Target Platform:** ESP32 Microcontroller (Wokwi Emulation + Adafruit IO Cloud)
- **Live Simulation Link:** [https://wokwi.com/projects/465449721853605889](https://wokwi.com/projects/465449721853605889)
- **Adafruit IO Dashboards:**
  - Facility Management: [https://io.adafruit.com/Sobhita/dashboards/facility-management-view](https://io.adafruit.com/Sobhita/dashboards/facility-management-view)
  - Medical Staff View: [https://io.adafruit.com/Sobhita/dashboards/medical-staff-view](https://io.adafruit.com/Sobhita/dashboards/medical-staff-view)

---

## 🎯 Verification Checklist

### 1. Source Code & Architecture
- [x] **Source Code Organization:** Complete firmware code placed under `src/sketch.ino` with clear modular organization and comments.
- [x] **Hardware Circuit Schematic:** Wokwi circuit diagram present under `src/diagram.json`.
- [x] **Library Manifest:** Required libraries specified in `src/libraries.txt`.
- [x] **Dual-Core FreeRTOS Architecture:** Multi-tasking properly divided between Core 1 (Sensors) and Core 0 (UI & MQTT) using FreeRTOS mailbox queues.
- [x] **No Blocking Delays:** Elapsed-time patterns (`millis()`) used throughout the runtime loop to maintain non-blocking execution.

### 2. Monitoring & Actuation Requirements
- [x] **Vital Signs Monitored:** Heart Rate (HR), Blood Oxygen Saturation (SpO2), and Body Temperature.
- [x] **Environmental Metrics Monitored:** Room Temperature, Room Oxygen (O2), and Air Quality Index (AQI).
- [x] **Remote Actuation Implemented:**
  - Remote IV Dosage Adjustment via Adafruit IO slider with local warning LED (>80 mg).
  - Remote Bed Elevation Angle Control (0°–180°) mapped to servo motor.
  - Dynamic Telemetry Publish Rate adjustment (5 s to 60 s).
- [x] **Fault Injection Bank:** 5 DIP switches implemented for Fever, Tachycardia, Hypoxia, Motion, and Network Kill Switch.
- [x] **Multi-Zone Alarms:** 5 distinct buzzer channels and status flags for multi-condition alert generation.
- [x] **Offline Failsafe:** Autonomous offline detection, audible pulsed alarm, OLED status update, and non-blocking auto-reconnect backoff.

### 3. Mandatory Documentation & Deliverables
- [x] **README.md:** Fully informative, detailing project overview, pinout tables, FreeRTOS task graphs, MQTT feed specifications, and setup instructions.
- [x] **Bug Log (BUG_LOG.md):** 8 detailed bug reports documenting severity, root causes, and verified fixes.
- [x] **Assumptions Document (ASSUMPTIONS.md):** Comprehensive breakdown of hardware, clinical, network, and simulation assumptions.
- [x] **Self-Attestation Checklist (SELF_ATTESTATION.md):** Completed and verified.
- [x] **Technical Report (docs/KSVS_Sobhita_Report.pdf):** 6-page comprehensive academic report.
- [x] **Video Demonstration (media/KSVS_Sobhita_Demo.mp4):** Full end-to-end video recording demonstrating live vitals, fault injection, and remote dashboard actuation.

### 4. Repository & Link Accessibility
- [x] **Public Accessibility:** Wokwi simulation link is public and testable directly in any modern browser without login.
- [x] **Cloud Dashboards:** Adafruit IO dashboards are publicly shared and accessible.
- [x] **Clean Repository Structure:** Organized folder hierarchy conforming to standard open-source and internship submission conventions.

---

## ✍️ Self-Attestation Statement

> *I hereby certify that the code, reports, documentation, and simulation models submitted within this repository are my own original work. All external libraries, platforms (Wokwi, Adafruit IO), and references have been properly cited and attributed in accordance with academic and professional integrity standards.*

**Signature:** KSVS Sobhita  
**Date:** March 2026  
