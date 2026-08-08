# ESP32 FreeRTOS Sensor System

A multitasking embedded system based on ESP32 and FreeRTOS.

The project demonstrates how multiple independent tasks can work simultaneously to read sensor data, update an LCD display, monitor alarm conditions and provide system diagnostics.

The project is simulated using Wokwi.

## Features

- ESP32
- FreeRTOS
- Multiple concurrent tasks
- DHT22 temperature and humidity sensor
- LCD 16x2 I2C display
- Alarm LED
- FreeRTOS Queue
- FreeRTOS Mutex
- Task priorities
- Periodic task execution
- System monitoring
- Wokwi simulation

## System Architecture

```text
                 ┌──────────────┐
                 │    DHT22     │
                 └──────┬───────┘
                        │
                        ▼
                ┌───────────────┐
                │  Sensor Task  │
                └───────┬───────┘
                        │
                     Queue
                        │
              ┌─────────┴─────────┐
              │                   │
              ▼                   ▼
       ┌──────────────┐    ┌──────────────┐
       │ Display Task │    │  Alarm Task  │
       └──────┬───────┘    └──────┬───────┘
              │                   │
              ▼                   ▼
          LCD 16x2             LED Alarm

                System Monitor Task
                        │
                        ▼
                   Serial Monitor
