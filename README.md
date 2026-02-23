# Smart Fridge Monitoring System (ESP32)

## Overview
An ESP32-based embedded monitoring system designed to track refrigerator temperature and detect prolonged door openings. The system provides real-time feedback via an LCD display, audible alerts, and a web interface for viewing historical temperature data.

This project demonstrates embedded firmware development, peripheral interfacing, real-time timing logic, and on-device web server implementation.

---

## Features
- Real-time temperature monitoring
- Door-open detection using pressure plate sensor
- Timer-based alert system with buzzer notification
- I2C LCD display for live temperature readings
- Embedded HTML web server for temperature history logging
- Serial debugging output for system diagnostics

---

## Hardware Components
- ESP32 DevKit
- Temperature sensor (AM2302)
- Ultrasonic sensor(HC-SR04)
- I2C LCD display
- Buzzer

---

## System Architecture

### Task Flow
1. Read temperature sensor
2. Monitor door state via GPIO
3. Start timer if door remains open
4. Trigger buzzer if timer threshold exceeded
5. Update LCD display
6. Serve historical data via web server

---

## Software Architecture

- Written in C/C++ (Arduino)
- Hardware timer logic for door-open threshold
- I2C communication for LCD display
- Embedded HTTP server for browser-based monitoring

---

## Concepts Demonstrated
- GPIO input monitoring
- Timer-based event handling
- I2C communication
- Embedded web server implementation
- FreeRTOS task scheduling


---

## Potential improvements
- Implement Wi-Fi reconnection logic
- Improve UI styling of web interface

---

## Author
Andrew Tat

