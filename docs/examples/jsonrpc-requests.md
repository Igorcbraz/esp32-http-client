---
title: JSON-RPC 2.0 Web Services Example - ESP32 HTTP Client
description: Complete guide and examples demonstrating JSON-RPC 2.0 requests, positional and named parameters, notifications, and struct mapping on ESP32.
keywords: JSON-RPC ESP32 example, JSON-RPC 2.0 Arduino, JSON-RPC client ESP32, JsonRpcRequest
tags:
  - examples
  - jsonrpc
  - tutorial
---
# Consuming JSON-RPC 2.0 Web Services

`ESP32-HTTP-Client` provides native, zero-heap streaming support for **JSON-RPC 2.0** over HTTP and HTTPS. It automatically serializes JSON-RPC 2.0 request envelopes, supports positional and named parameters, handles notifications, and extracts result payloads directly into C++ variables and mapped structs.

---

## 1. Positional Parameters (Array Params)

In JSON-RPC 2.0, when parameters are ordered as an array (`"params": [15, 27]`), use `.param(value)` or `.addParam(value)`:

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://api.example.com");

void setup() {
  Serial.begin(115200);
  WiFi.begin("SSID", "PASS");
  while (WiFi.status() != WL_CONNECTED) delay(500);

  int sum = 0;

  // Sends: {"jsonrpc":"2.0","method":"add","params":[15,27],"id":1}
  client.jsonRpc("/rpc")
        .method("add")
        .param(15)
        .param(27)
        .id(1)
        .getResult(&sum);

  if (client.isSuccess()) {
    Serial.printf("Sum result: %d\n", sum);
  }
}

void loop() {}
```

---

## 2. Named Parameters (Object Params)

When the server expects named parameters (`"params": {"minuend": 42, "subtrahend": 23}`), use `.param(key, value)` or `.setParam(key, value)`:

```cpp
int subResult = 0;

// Sends: {"jsonrpc":"2.0","method":"subtract","params":{"minuend":42,"subtrahend":23},"id":"sub-1"}
client.jsonRpc("/rpc")
      .method("subtract")
      .param("minuend", 42)
      .param("subtrahend", 23)
      .id("sub-1")
      .getResult(&subResult);

Serial.printf("Subtraction result: %d\n", subResult);
```

---

## 3. Notifications (Fire-and-Forget)

Per the JSON-RPC 2.0 specification, a **Notification** is a request without an `id` member. The client does not expect or wait to parse a response:

```cpp
// Sends: {"jsonrpc":"2.0","method":"logTelemetry","params":{"node":"esp32","uptime":120000}}
client.jsonRpc("/rpc")
      .method("logTelemetry")
      .asNotification()
      .param("node", "esp32")
      .param("uptime", 120000);
```

---

## 4. Struct Mapping with `REST_JSON_MAP`

You can pass C++ `struct`s directly into JSON-RPC parameters or bind the result object directly into a struct without dynamic JSON documents:

```cpp
struct SensorData {
  char sensor[32] = {0};
  float temperature = 0.0f;
  float humidity = 0.0f;

  REST_JSON_MAP(
    REST_FIELD(sensor),
    REST_FIELD(temperature),
    REST_FIELD(humidity)
  )
};

// 1. Send struct as params object:
SensorData report = {"DHT22", 24.5f, 55.0f};
client.jsonRpc("/rpc")
      .method("saveReading")
      .setParams(report)
      .id(101);

// 2. Deserializing result into a struct:
SensorData latestReading;
client.jsonRpc("/rpc")
      .method("getLatestReading")
      .param("sensorId", 1)
      .id(102)
      .getResult(&latestReading);

Serial.printf("Sensor: %s, Temp: %.1f C\n", latestReading.sensor, latestReading.temperature);
```

---

## 5. Error Handling and Standard Error Codes

JSON-RPC 2.0 returns errors inside the `error` object (`{"code": -32601, "message": "Method not found"}`). The library provides standard error code constants and error inspection:

```cpp
JsonRpcError rpcError;
int output = 0;

client.jsonRpc("/rpc")
      .method("nonExistentMethod")
      .id(5)
      .getError(&rpcError)
      .onJsonRpcError([](const JsonRpcError& err) {
          Serial.printf("JSON-RPC Error [%d]: %s\n", err.code, err.message.c_str());
      })
      .getResult(&output);

if (rpcError.code == JSONRPC_ERR_METHOD_NOT_FOUND) {
  Serial.println("Requested method was not found on server.");
}
```

### Standard JSON-RPC Error Codes

| Constant | Code | Meaning |
| :--- | :--- | :--- |
| `JSONRPC_ERR_PARSE_ERROR` | `-32700` | Invalid JSON was received by the server. |
| `JSONRPC_ERR_INVALID_REQUEST` | `-32600` | The JSON sent is not a valid Request object. |
| `JSONRPC_ERR_METHOD_NOT_FOUND` | `-32601` | The method does not exist or is not available. |
| `JSONRPC_ERR_INVALID_PARAMS` | `-32602` | Invalid method parameter(s). |
| `JSONRPC_ERR_INTERNAL_ERROR` | `-32603` | Internal JSON-RPC error on server. |
| `JSONRPC_ERR_SERVER_ERROR_START` to `END` | `-32099` to `-32000` | Reserved for implementation-defined server errors. |

---

## 6. Extracting Nested Fields and Raw JSON

Use dot notation to bind nested properties of the result object, or capture the raw JSON string:

```cpp
char city[64] = {0};
String rawResult;
String rawHttpBody;

client.jsonRpc("/rpc")
      .method("getUser")
      .param("id", 42)
      .id("u42")
      .getResult("address.city", city, sizeof(city)) // Extract nested result field
      .getRawResult(&rawResult)                     // Extract entire "result" object/array as raw JSON
      .getRawResponse(&rawHttpBody);                 // Extract complete HTTP response body
```

---

## 7. Full Working Sketch

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://api.example.com");

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Optional: Bearer authentication or custom headers
  client.bearer("your_api_token");

  // 1. Call with positional parameters
  int sum = 0;
  client.jsonRpc("/rpc")
        .method("add")
        .param(15)
        .param(27)
        .id(1)
        .getResult(&sum);

  Serial.printf("add(15, 27) = %d\n", sum);

  // 2. Call with named parameters and error handling
  int diff = 0;
  client.jsonRpc("/rpc")
        .method("subtract")
        .param("minuend", 50)
        .param("subtrahend", 12)
        .id("sub-1")
        .getResult(&diff)
        .onJsonRpcError([](const JsonRpcError& err) {
            Serial.printf("Error [%d]: %s\n", err.code, err.message.c_str());
        });

  Serial.printf("subtract(50, 12) = %d\n", diff);

  // 3. Notification (no id, fire-and-forget)
  client.jsonRpc("/rpc")
        .method("heartbeat")
        .asNotification()
        .param("node", "esp32");
}

void loop() {}
```
