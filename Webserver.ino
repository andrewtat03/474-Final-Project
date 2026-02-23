/**
 * @file Webserver.ino
 * @brief ESP32 Web Server with Alert and LCD Display Functionality
 *
 * This program sets up an ESP32 as a Wi-Fi access point and hosts a web server
 * to monitor temperature and freezer distance values. It also includes alert handling for 
 * abnormal temperature and distance, displaying alerts LED notifications, and 
 * LCD display updates using FreeRTOS tasks and timers.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "WebserverHTML.h"
#include <LiquidCrystal_I2C.h>

/** @brief Wi-Fi access point SSID */
#define AP_SSID "ESP32_AccessPoint"
/** @brief Wi-Fi access point password */
#define AP_PASS "12345678"

/** @brief GPIO pin for LED alerts */
#define LED_PIN 19

/** @brief Web server instance on port 80 */
WebServer server(80);

/** @brief FreeRTOS task handle for the web server */
TaskHandle_t serverTaskHandle;
/** @brief FreeRTOS software timer handle for alert checking */
TimerHandle_t AlertTaskTimer;
/** @brief FreeRTOS software timer handle for LED alert */
TimerHandle_t LedAlertTaskTimer;

/** @brief Measured temperature value */
volatile float temperature = 85.0;
/** @brief Measured distance value */
volatile float distance = 20.0;
/** @brief Alert message for display and web server */
String alertMessage = "";

/** @brief LCD display instance */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/** @brief Temperature threshold for alerts in Fahrenheit */
#define TEMP_THRESHOLD 32.0
/** @brief Distance threshold for alerts in Centimeters*/
#define DIST_THRESHOLD 100.0

/** @brief Web server task function */
void ServerTask(void* pvParameters);
/** @brief Periodic alert checking task */
void CheckAlertsTask(void* pvParameters);
/** @brief Handles incoming POST requests from second ESP32 */
void handlePostRequest();

/**
 * @brief Initializes Wi-Fi, web server, tasks, and timers.
 */
void setup() {
    Serial.begin(115200);

    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("ESP32 Access Point IP Address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", []() {
        String html = PAGE_MAIN;
        html.replace("{{temperature}}", String(temperature, 1));
        html.replace("{{distance}}", String(distance, 1));
        html.replace("{{alertMessage}}", alertMessage);
        server.send(200, "text/html", html);
    });

    server.on("/update", HTTP_POST, handlePostRequest);
    server.begin();

    xTaskCreatePinnedToCore(ServerTask, "ServerTask", 4096, NULL, 1, &serverTaskHandle, 0);

    AlertTaskTimer = xTimerCreate("Alert Task timer", pdMS_TO_TICKS(20), pdTRUE, 0, Alert_Task_TimerCallback);
    if (AlertTaskTimer != NULL) {
      xTimerStart(AlertTaskTimer, 0);
    }

    LedAlertTaskTimer = xTimerCreate("Led Alert Task timer", pdMS_TO_TICKS(31), pdTRUE, 0, Led_Alert_Task_TimerCallback);
    if (LedAlertTaskTimer != NULL) {
      xTimerStart(LedAlertTaskTimer, 0);
    }

    pinMode(LED_PIN, OUTPUT);
    xTaskCreate(displayLCDTask, "LCD Display task", 4096, NULL, 1, NULL);
}

/**
 * @brief Main loop (not used as tasks are FreeRTOS-based).
 */
void loop() {}

/**
 * @brief Web server task to handle incoming client requests.
 * @param pvParameters Task parameters (unused).
 */
void ServerTask(void *pvParameters) {
    while (true) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Callback function for the alert checking timer.
 * @param xTimer Timer handle (unused).
 */
void Alert_Task_TimerCallback(TimerHandle_t xTimer) {
    String newAlert = "";

    if (temperature > TEMP_THRESHOLD) {
        newAlert = "Temperature is not correct!";
    } else if (distance > DIST_THRESHOLD) {
        newAlert = "Freezer is open!";
    }

    alertMessage = newAlert;
}

/**
 * @brief Callback function for the LED alert timer.
 * @param xTimer Timer handle (unused).
 */
void Led_Alert_Task_TimerCallback(TimerHandle_t xTimer) {
    if (temperature > TEMP_THRESHOLD || distance > DIST_THRESHOLD) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
}

/**
 * @brief Handles HTTP POST requests to update sensor values.
 */
void handlePostRequest() {
    if (server.hasArg("temperature") && server.hasArg("distance")) {
        temperature = server.arg("temperature").toFloat();
        distance = server.arg("distance").toFloat();

        if (distance > 300) {
          distance = 300;
        }
        server.send(200, "text/plain", "Data received");
        Serial.println("Received Temp: " + String(temperature) + " °F, Distance: " + String(distance) + " cm");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

/**
 * @brief Task to update the LCD display with temperature and distance values.
 * @param pvParams Task parameters (unused).
 */
void displayLCDTask(void* pvParams) {
  lcd.init();
  lcd.backlight();
  while (1) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Temp: ");
      lcd.print(temperature);
      lcd.setCursor(0,1);
      lcd.print("Distance: ");
      lcd.print(distance);
      vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
