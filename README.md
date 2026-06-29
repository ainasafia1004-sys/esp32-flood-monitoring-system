# Project Overview

This project is an intelligent, IoT-based Flood Monitoring and Early Warning System designed to track real-time environmental parameters at remote monitoring nodes. Tailored to the local environmental context, the system measures critical weather metrics to analyze, detect, and broadcast potential flood risks before they escalate.

# Key Features & Architecture

The system is built on a powerful microcontroller architecture integrated with dual-platform cloud services and automated emergency broadcasting:Multi-Sensor Telemetry Data Collection: * Uses a DHT11 sensor to acquire real-time ambient temperature and humidity readings.Utilizes a Hall Effect Sensor paired with a custom anemometer setup to calculate rotational speed (RPM), which is then processed to determine actual wind speed in kilometers per hour ($km/h$).Dual-Platform Cloud Integration: Telemetry data is simultaneously broadcasted to two distinct IoT infrastructures via WiFi:Blynk IoT Console: Provides a responsive mobile and web interface for instant hardware data tracking.Adafruit IO Dashboard: Acts as a public Community Climate Monitor. It features a dynamic Color Picker block that serves as a live visual triage indicator, shifting colors automatically based on the risk level (Green = Safe, Yellow = Beware, Red = Danger).Edge Analytics & Instant Emergency Alerts: The system actively monitors threshold constraints. If any sensor breaches its defined critical limit, it immediately triggers edge-level logic, flashes a hardware pulse to an ESP32-CAM module for station visualization, and instantly dispatches an automated, formatted Emergency Alert Report to a dedicated Telegram Channel for immediate community awareness.

# Project Objective

The ultimate goal of this project is to bridge the gap between environmental telemetry and community safety. By providing localized, accurate, and rapid early warnings, the system empowers residents to take preemptive actions—such as clearing drainage areas and securing assets—thereby minimizing property damage and safeguarding lives.

# esp32-flood-monitoring-system
ESP32 Flood Monitoring System using Hall Effect Sensor, DHT11 &amp; ESP32-Cam

## Features
- RPM calculation
- Wind speed in m/s and km/h
- Serial Monitor output

## Hardware used
- ESP32
- Hall Effect Sensor
- Magnet
- DHT11

## Author
Aina Safia 
