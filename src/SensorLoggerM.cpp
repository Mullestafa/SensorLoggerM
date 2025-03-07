#include "SensorLoggerM.h"
#include <ArduinoJson.h>

SensorLoggerM::SensorLoggerM(const char* serverUrl) {
  _serverUrl = String(serverUrl);
  // Create the mutex for protecting the log buffer.
  _mutex = xSemaphoreCreateMutex();
}

SensorLoggerM::~SensorLoggerM() {
  if (_mutex != nullptr) {
    vSemaphoreDelete(_mutex);
  }
}

bool SensorLoggerM::begin(const char* ssid, const char* password, unsigned long timeout) {
  WiFi.begin(ssid, password);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
    delay(500);
  }
  return (WiFi.status() == WL_CONNECTED);
}

void SensorLoggerM::log(const String &experimentId, const String &deviceName, const String &sensorName, float value) {
  SensorLog entry;
  entry.experimentId = experimentId;
  entry.deviceName = deviceName;
  entry.sensorName = sensorName;
  entry.value = value;
  entry.timestamp = millis(); // You could also use an RTC or NTP for a real timestamp.

  // Take the mutex before modifying the buffer.
  if(xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
    _logBuffer.push_back(entry);
    xSemaphoreGive(_mutex);
  }
}

bool SensorLoggerM::flush() {
  std::vector<SensorLog> logsToSend;

  // Lock the buffer to copy its contents and then clear it.
  if(xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }
  if (_logBuffer.empty()) {
    xSemaphoreGive(_mutex);
    return true;
  }
  logsToSend = _logBuffer;
  _logBuffer.clear();
  xSemaphoreGive(_mutex);

  // Build the JSON payload from the copied logs.
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  for (auto &entry : logsToSend) {
    JsonObject obj = array.createNestedObject();
    obj["experiment_id"] = entry.experimentId;
    obj["device_name"] = entry.deviceName;
    obj["sensor_name"] = entry.sensorName;
    obj["value"] = entry.value;
    obj["timestamp"] = entry.timestamp;
  }
  String payload;
  serializeJson(doc, payload);

  // Send the JSON payload using HTTP.
  HTTPClient http;
  http.begin(_serverUrl);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(payload);
  bool success = false;
  if (httpResponseCode > 0) {
    success = true;
  } else {
    Serial.print("Error sending logs: ");
    Serial.println(http.errorToString(httpResponseCode));
    // On failure, reinsert the logs back into the buffer.
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
      _logBuffer.insert(_logBuffer.begin(), logsToSend.begin(), logsToSend.end());
      xSemaphoreGive(_mutex);
    }
  }

  http.end();
  return success;
}

void SensorLoggerM::clearBuffer() {
  if(xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
    _logBuffer.clear();
    xSemaphoreGive(_mutex);
  }
}
