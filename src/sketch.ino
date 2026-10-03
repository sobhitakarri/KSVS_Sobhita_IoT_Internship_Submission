#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ESP32Servo.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// --- HARDWARE PIN DEFINITIONS ---
#define TEMP_SENSOR 14
#define MOTION_DETECTOR 12

// Outputs (Actuators & Alarms)
#define BUZZER_PIN_1 4     // Body Temp Alarm
#define BUZZER_PIN_2 16    // HR/BP Alarm
#define BUZZER_PIN_3 17    // SpO2/Resp Alarm
#define BUZZER_PIN_4 5     // Env/Room Temp Alarm
#define OFFLINE_BUZZER 19  // Network Failsafe Alarm
#define IV_LED_PIN 27      // High IV Dosage Warning
#define SERVO_PIN 18       // Bed Elevation Motor

// Inputs (Fault Injection Switches & Pots)
#define KILL_SWITCH 13     // DIP Switch 1: Simulates Network Crash
#define SW_TEMP 32         // DIP Switch 8: Simulates Fever
#define SW_HR 33           // DIP Switch 7: Simulates Tachycardia
#define SW_SPO2 25         // DIP Switch 6: Simulates Hypoxia
#define SW_MOTION 26       // DIP Switch 5: Motion Bypass

#define PIN_O2 35          // Potentiometer: Room Oxygen
#define PIN_AQI 39         // Potentiometer: Air Quality Index

// OLED Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
#define OLED_ADDRESS 0x3C

// --- WIFI & MQTT CREDENTIALS ---
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""
#define IO_USERNAME  "Sobhita"
#define IO_KEY       "YOUR_AIO_KEY"  // Replace with your Adafruit IO Key
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883

// --- MQTT FEEDS ---
#define HR_FEED          IO_USERNAME "/feeds/heart-rate-monitor"
#define SPO2_FEED        IO_USERNAME "/feeds/spo2"
#define BODY_TEMP_FEED   IO_USERNAME "/feeds/body-temp"
#define ROOM_TEMP_FEED   IO_USERNAME "/feeds/room-temp"
#define ROOM_O2_FEED     IO_USERNAME "/feeds/room-oxygen"
#define AQI_FEED         IO_USERNAME "/feeds/air-quality"
#define MOTION_FEED      IO_USERNAME "/feeds/motion-detector"
#define FLAG             IO_USERNAME "/feeds/flag"

#define DOSAGE_FEED      IO_USERNAME "/feeds/dosage"
#define BED_FEED         IO_USERNAME "/feeds/bed-elevation"
#define RATE_FEED        IO_USERNAME "/feeds/publish-rate"

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, IO_USERNAME, IO_KEY);

Adafruit_MQTT_Publish hrFeed             = Adafruit_MQTT_Publish(&mqtt, HR_FEED);
Adafruit_MQTT_Publish spo2Feed           = Adafruit_MQTT_Publish(&mqtt, SPO2_FEED);
Adafruit_MQTT_Publish bodyTempFeed       = Adafruit_MQTT_Publish(&mqtt, BODY_TEMP_FEED);
Adafruit_MQTT_Publish roomTempFeed       = Adafruit_MQTT_Publish(&mqtt, ROOM_TEMP_FEED);
Adafruit_MQTT_Publish roomO2Feed         = Adafruit_MQTT_Publish(&mqtt, ROOM_O2_FEED);
Adafruit_MQTT_Publish aqiFeed            = Adafruit_MQTT_Publish(&mqtt, AQI_FEED);
Adafruit_MQTT_Publish motiondetectorFeed = Adafruit_MQTT_Publish(&mqtt, MOTION_FEED);
Adafruit_MQTT_Publish statusFeed         = Adafruit_MQTT_Publish(&mqtt, FLAG);

Adafruit_MQTT_Subscribe dosageSub = Adafruit_MQTT_Subscribe(&mqtt, DOSAGE_FEED);
Adafruit_MQTT_Subscribe bedSub    = Adafruit_MQTT_Subscribe(&mqtt, BED_FEED); 
Adafruit_MQTT_Subscribe rateSub   = Adafruit_MQTT_Subscribe(&mqtt, RATE_FEED); 

OneWire oneWire(TEMP_SENSOR);
DallasTemperature sensors(&oneWire);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Servo bedServo; 

QueueHandle_t roomTempQueue, hrQueue, spo2Queue, bodyTempQueue, roomO2Queue, aqiQueue;

int currentDosage = 0; 
int currentBedAngle = 0; 
unsigned long publishInterval = 20000; 

// --- CORE 1: HARDWARE SENSOR TASK ---
void sensorTask(void *param) {
  float roomTemp = 0.0;
  int heartRate = 0, spo2 = 0;

  while (1) {
    sensors.requestTemperatures();
    roomTemp = sensors.getTempCByIndex(0);
    if (roomTemp <= -100.0 || roomTemp == 0.0) roomTemp = 24.0 + (random(-10, 10) / 10.0);

    if (digitalRead(SW_HR) == LOW) heartRate = random(110, 135); 
    else heartRate = random(65, 85);   

    if (digitalRead(SW_SPO2) == LOW) spo2 = random(82, 88); 
    else spo2 = random(95, 100); 

    xQueueOverwrite(roomTempQueue, &roomTemp);
    xQueueOverwrite(hrQueue, &heartRate);
    xQueueOverwrite(spo2Queue, &spo2);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// --- CORE 1: ENVIRONMENT & SIMULATION TASK ---
void advancedSensorTask(void *param) {
  int roomO2 = 0, aqi = 0;
  float bodyTemp = 0.0;

  while (1) {
    roomO2 = map(analogRead(PIN_O2), 0, 4095, 15, 25);
    aqi = map(analogRead(PIN_AQI), 0, 4095, 0, 500);

    if (digitalRead(SW_TEMP) == LOW) bodyTemp = 38.5 + (random(0, 10) / 10.0); 
    else bodyTemp = 36.5 + (random(0, 7) / 10.0);  

    xQueueOverwrite(roomO2Queue, &roomO2);
    xQueueOverwrite(aqiQueue, &aqi);
    xQueueOverwrite(bodyTempQueue, &bodyTemp);

    vTaskDelay(pdMS_TO_TICKS(2000)); 
  }
}

// --- CORE 0: DISPLAY, ALARMS & MQTT TASK ---
void processingTask(void *param) {
  float roomTemp = 0.0, bodyTemp = 0.0; 
  int heartRate = 0, spo2 = 0, roomO2 = 0, aqi = 0;
  const char *statusMsg = "System OK";
  
  unsigned long lastPublishTime = 0; 
  unsigned long lastReconnectAttempt = 0; 
  bool isOffline = false; 

  vTaskDelay(pdMS_TO_TICKS(3000)); 

  while (1) {
    // 1. OFFLINE DETECTION & KILL SWITCH
    bool killSwitchActive = (digitalRead(KILL_SWITCH) == LOW);
    
    if (WiFi.status() != WL_CONNECTED || !mqtt.connected() || killSwitchActive) {
      isOffline = true;
      statusMsg = "LOGGING OFFLINE";

      if (killSwitchActive && mqtt.connected()) mqtt.disconnect();

      digitalWrite(OFFLINE_BUZZER, HIGH);
      vTaskDelay(pdMS_TO_TICKS(100)); 
      digitalWrite(OFFLINE_BUZZER, LOW);

      if (!killSwitchActive && (millis() - lastReconnectAttempt > 5000)) {
        if (WiFi.status() == WL_CONNECTED) mqtt.connect();
        else WiFi.reconnect();
        lastReconnectAttempt = millis();
      }
    } else {
      isOffline = false;
      statusMsg = "System OK"; 
    }

    // 2. READ REMOTE COMMANDS (If Online)
    if (!isOffline) {
      Adafruit_MQTT_Subscribe *subscription;
      while ((subscription = mqtt.readSubscription(200))) {
        if (subscription == &dosageSub) currentDosage = atoi((char *)dosageSub.lastread);
        else if (subscription == &bedSub) {
          currentBedAngle = atoi((char *)bedSub.lastread);
          bedServo.write(currentBedAngle); 
        }
        else if (subscription == &rateSub) publishInterval = atoi((char *)rateSub.lastread) * 1000UL; 
      }
    }

    // 3. READ LOCAL SENSORS
    xQueuePeek(roomTempQueue, &roomTemp, 0);
    xQueuePeek(hrQueue, &heartRate, 0);
    xQueuePeek(spo2Queue, &spo2, 0);
    xQueuePeek(roomO2Queue, &roomO2, 0);
    xQueuePeek(aqiQueue, &aqi, 0);
    xQueuePeek(bodyTempQueue, &bodyTemp, 0);

    // 4. ALARM LOGIC
    if (!isOffline) {
      if (currentDosage > 80) {
        statusMsg = "WARN: High IV Dose!";
        digitalWrite(IV_LED_PIN, HIGH); 
      } else {
        digitalWrite(IV_LED_PIN, LOW); 
      }

      if (bodyTemp > 37.8) statusMsg = "Vitals: Fever!";
      else if (heartRate > 100) statusMsg = "Vitals: High HR!";
      else if (spo2 > 0 && spo2 < 90) statusMsg = "Vitals: Low SpO2!";
      else if (roomTemp > 28) statusMsg = "Env: Room Too Hot!";
      else if (roomO2 > 0 && (roomO2 < 19 || roomO2 > 23)) statusMsg = "Env: O2 Hazard!";
      else if (aqi > 100) statusMsg = "Env: Poor Air Quality!";
    }

    // 5. OLED DASHBOARD
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("HR:"); display.print(heartRate); 
    display.print(" O2:"); display.print(spo2); display.print("% T:"); display.print(bodyTemp, 1);

    display.setCursor(0, 16);
    display.print("Env: "); display.print(roomTemp, 1); display.print("C AQI:"); display.print(aqi);

    display.setCursor(0, 32);
    display.print("IV:"); display.print(currentDosage); display.print("mg | Bed:"); display.print(currentBedAngle); display.print("deg");

    display.setCursor(0, 48);
    display.print(statusMsg);
    
    if (!isOffline) {
      display.setCursor(100, 48);
      display.print(publishInterval / 1000); display.print("s"); 
    }

    display.display();

    // 6. DYNAMIC PUBLISH (If Online)
    if (!isOffline && (millis() - lastPublishTime >= publishInterval)) {
      mqtt.ping(); 
      if (heartRate > 0) hrFeed.publish((int32_t)heartRate);
      if (spo2 > 0) spo2Feed.publish((int32_t)spo2);
      if (bodyTemp > 30.0) bodyTempFeed.publish(bodyTemp);
      if (roomTemp > 10.0) roomTempFeed.publish(roomTemp);
      roomO2Feed.publish((int32_t)roomO2);
      aqiFeed.publish((int32_t)aqi);
      statusFeed.publish(statusMsg);
      lastPublishTime = millis(); 
    }
    vTaskDelay(pdMS_TO_TICKS(250)); 
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN_1, OUTPUT);
  pinMode(BUZZER_PIN_2, OUTPUT);
  pinMode(BUZZER_PIN_3, OUTPUT);
  pinMode(BUZZER_PIN_4, OUTPUT);
  pinMode(OFFLINE_BUZZER, OUTPUT); 
  pinMode(IV_LED_PIN, OUTPUT);
  pinMode(MOTION_DETECTOR, INPUT);

  pinMode(KILL_SWITCH, INPUT_PULLUP);
  pinMode(SW_TEMP, INPUT_PULLUP);
  pinMode(SW_HR, INPUT_PULLUP);
  pinMode(SW_SPO2, INPUT_PULLUP);
  pinMode(SW_MOTION, INPUT_PULLUP);

  sensors.begin();
  Wire.begin(21, 22);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();

  bedServo.attach(SERVO_PIN);
  bedServo.write(0); 

  // Fast, non-blocking WiFi start
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 10) {
    delay(500);
    attempt++;
  }

  mqtt.subscribe(&dosageSub);
  mqtt.subscribe(&bedSub);
  mqtt.subscribe(&rateSub); 

  roomTempQueue = xQueueCreate(1, sizeof(float));
  hrQueue       = xQueueCreate(1, sizeof(int));
  spo2Queue     = xQueueCreate(1, sizeof(int));
  bodyTempQueue = xQueueCreate(1, sizeof(float));
  roomO2Queue   = xQueueCreate(1, sizeof(int));
  aqiQueue      = xQueueCreate(1, sizeof(int));

  xTaskCreatePinnedToCore(advancedSensorTask, "AdvSensorTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(processingTask, "ProcessTask", 6144, NULL, 2, NULL, 0);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}