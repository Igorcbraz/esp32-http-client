#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://api.example.com");

struct SensorStatus {
  int battery;
  float temperature;
  String mode;

  REST_JSON_MAP(
    REST_FIELD(battery),
    REST_FIELD(temperature),
    REST_FIELD(mode)
  )
};

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void loop() {
  Serial.println("\n--- [1] JSON-RPC Request with Positional Parameters ---");
  int sumResult = 0;

  client.jsonRpc("/rpc")
      .method("add")
      .param(15)
      .param(27)
      .id(1)
      .getResult(&sumResult);

  if (client.isSuccess()) {
    Serial.printf("Sum result: %d\n", sumResult);
  } else {
    Serial.printf("HTTP Error: %d - %s\n", client.getStatusCode(), client.getErrorMessage().c_str());
  }

  Serial.println("\n--- [2] JSON-RPC Request with Named Parameters ---");
  int subResult = 0;

  client.jsonRpc("/rpc")
      .method("subtract")
      .param("minuend", 100)
      .param("subtrahend", 42)
      .id("req-subtract-01")
      .getResult(&subResult);

  if (client.isSuccess()) {
    Serial.printf("Subtract result: %d\n", subResult);
  }

  Serial.println("\n--- [3] JSON-RPC Notification (no response expected) ---");
  client.jsonRpc("/rpc")
      .method("logEvent")
      .asNotification()
      .param("device", "ESP32")
      .param("event", "heartbeat");

  Serial.println("Notification sent.");

  Serial.println("\n--- [4] JSON-RPC Struct Serialization & Result Binding ---");
  SensorStatus currentStatus{95, 23.4f, "active"};
  SensorStatus updatedStatus{0, 0.0f, ""};

  client.jsonRpc("/rpc")
      .method("updateSensorStatus")
      .id(2)
      .params(currentStatus)
      .getResult(&updatedStatus);

  if (client.isSuccess()) {
    Serial.printf("Updated Status: Battery=%d%%, Temp=%.1fC, Mode=%s\n",
                  updatedStatus.battery, updatedStatus.temperature, updatedStatus.mode.c_str());
  }

  delay(15000);
}
