#ifndef SENSOR_LOGGER_M_H
#define SENSOR_LOGGER_M_H

#include <Arduino.h>
#include <WiFi.h>         // For ESP32; for ESP8266, use <ESP8266WiFi.h>
#include <HTTPClient.h>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Structure to hold a sensor log entry.
struct SensorLog {
  String experimentId;
  String deviceName;
  String sensorName;
  float value;
  unsigned long timestamp;
};

class SensorLoggerM {
public:
  // Constructor: specify the server endpoint URL
  SensorLoggerM(const char* serverUrl);
  
  // Destructor: clean up the mutex.
  ~SensorLoggerM();
  
  // Connect to WiFi; returns true if connected successfully.
  bool begin(const char* ssid, const char* password, unsigned long timeout = 10000);
  
  // Log a sensor reading (thread‑safely adds an entry to the internal buffer).
  void log(const String &experimentId, const String &deviceName, const String &sensorName, float value);
  
  // Send the buffered logs to the server as a JSON array.
  // If successful, clears the buffer and returns true.
  bool flush();

  // Clear the internal buffer manually.
  void clearBuffer();
  
private:
  String _serverUrl;
  std::vector<SensorLog> _logBuffer;
  SemaphoreHandle_t _mutex;   // FreeRTOS mutex for thread safety.
};

#endif

