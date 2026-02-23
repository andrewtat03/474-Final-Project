/**
 * @file SensorTasks.cpp
 * @brief ESP32 tasks for reading sensors and sending data to a webserver hosted by another ESP32.
 *
 * This file includes tasks for reading temperature and distance using an AM2320 sensor
 * and an ultrasonic sensor, and then sending the data to a web server over WiFi.
 */

#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>

/** WiFi credentials (from other ESP32) */
#define WIFI_SSID "ESP32_AccessPoint"
#define WIFI_PASS "12345678"

/** Server IP address */
#define SERVER_IP "192.168.4.1"

/** GPIO pins for the ultrasonic sensor */
#define TRIGGER_PIN 5
#define ECHO_PIN 6

/** Sensor thresholds */
#define TEMP_THRESHOLD 0
#define DISTANCE_THRESHOLD 20

/** Moving average window sizes */
#define TEMP_WINDOW 10
#define DIST_WINDOW 100

/** AM2320 temperature and humidity sensor instance */
Adafruit_AM2320 am2320 = Adafruit_AM2320();

/** Buffers for moving average calculations */
double tempReadings[TEMP_WINDOW];
double distReadings[DIST_WINDOW];
int tempIndex, tempCount, distIndex, distCount;

/** Task handles */
TaskHandle_t UltrasonicHandle, TemperatureHandle, SenderHandle;

/** Queue handles */
QueueHandle_t TQHandle, UQHandle;

/**
 * @brief Reads temperature from AM2320 sensor and stores a moving average.
 *
 * This FreeRTOS task continuously reads the temperature, converts it to Fahrenheit,
 * maintains a moving average, and sends the average temperature to a queue.
 */
void TemperatureReadTask(void *pvParameters) {
  while (1) {
    // convert from Celsius to Fahrenheit
    double fahrenTemp = (am2320.readTemperature() * 1.8) + 32;
    tempReadings[tempIndex] = fahrenTemp;
    tempIndex = (tempIndex + 1) % TEMP_WINDOW;
    if (tempCount < TEMP_WINDOW) tempCount++;

    double average = 0.0;
    for (int i = 0; i < tempCount; i++) {
      average += tempReadings[i];
    }
    // checks for division by zero errors
    // if we haven't recorded any data yet, then have average be -1
    average = (tempCount != 0) ? average / tempCount : -1;

    xQueueSend(TQHandle, &average, 100);
    vTaskDelay(1000);
  }
}

/**
 * @brief Reads distance from the ultrasonic sensor and stores a moving average.
 *
 * This FreeRTOS task continuously reads distance measurements using an HC-SR04 sensor,
 * maintains a moving average, and sends the average distance to a queue.
 */
void UltrasonicReadTask(void *pvParameters) {
  while (1) {
    long duration;
    double distance;
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);
    // we use speed = distance / time formula her
    distance = (duration / 2) / 29.1;

    distReadings[distIndex] = distance;
    distIndex = (distIndex + 1) % DIST_WINDOW;
    if (distCount < DIST_WINDOW) distCount++;

    // checks for division by zero errors
    // if we haven't recorded any data yet, then have average be -1
    double average = 0.0;
    for (int i = 0; i < distCount; i++) {
      average += distReadings[i];
    }
    average = (distCount != 0) ? average / distCount : -2;

    xQueueSend(UQHandle, &average, 100);
    vTaskDelay(200);
  }
}

/**
 * @brief Sends sensor data to the server.
 *
 * This task retrieves data from the temperature and distance queues and sends
 * an HTTP POST request to the server with the latest readings.
 */
void sendData(void *pvParameters) {
  while (1) {
    double dataTemp, dataDist;
    xQueueReceive(TQHandle, &dataTemp, 100);
    xQueueReceive(UQHandle, &dataDist, 100);
    sendPostRequest(dataTemp, dataDist);
    vTaskDelay(1000);
  }
}

/**
 * @brief Sends an HTTP POST request with temperature and distance data.
 *
 * @param tempAverage The average temperature value to send.
 * @param distAverage The average distance value to send.
 */
void sendPostRequest(double tempAverage, double distAverage) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://" + String(SERVER_IP) + "/update";

    Serial.print("Sending POST request to: ");
    Serial.println(url);

    http.begin(url);
    // we need to add this to have a properly formatted HTTP request
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String postData = "temperature=" + String(tempAverage) + "&distance=" + String(distAverage);
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      Serial.print("Server Response Code: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.print("Error sending POST request: ");
      Serial.println(httpResponseCode);
    }

    // ends connection
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

/**
 * @brief Initializes the ESP32, connects to WiFi, and starts tasks.
 */
void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to ESP32 Hotspot");
  Serial.print("ESP32 Client IP: ");
  Serial.println(WiFi.localIP());
  am2320.begin();

  TQHandle = xQueueCreate(20, sizeof(double));
  UQHandle = xQueueCreate(20, sizeof(double));
  
  xTaskCreate(UltrasonicReadTask, "Ultrasonic Read", 4096, NULL, 1, &UltrasonicHandle);
  xTaskCreate(TemperatureReadTask, "Temperature Read", 4096, NULL, 1, &TemperatureHandle);
  xTaskCreate(sendData, "Send to server task", 4096, NULL, 1, &SenderHandle);
  Serial.print("Before Tasks");
}

/**
 * @brief Main loop (unused in FreeRTOS applications).
 */
void loop() {}
