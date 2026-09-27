#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://api.example.com");

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
  Serial.println("\n--- [1] JSON-RPC Error Handling with Callback ---");

  client.jsonRpc("/rpc")
      .method("nonExistentMethod")
      .id(1)
      .onJsonRpcError([](const JsonRpcError& err) {
        Serial.printf("[JSON-RPC Error Callback] Code: %d, Message: %s\n",
                      err.code, err.message.c_str());
        if (!err.data.isEmpty()) {
          Serial.printf("Additional Data: %s\n", err.data.c_str());
        }
      })
      .onSuccess([](int httpCode) {
        Serial.printf("Request succeeded with HTTP %d\n", httpCode);
      });

  Serial.println("\n--- [2] Inspecting JSON-RPC Error via Variables ---");

  JsonRpcError errorDetails;
  auto req = client.jsonRpc("/rpc");
  req.method("divide")
     .param("dividend", 10)
     .param("divisor", 0)
     .id(2)
     .getError(&errorDetails);

  req.execute();

  if (req.hasJsonRpcError()) {
    Serial.printf("Error detected: code=%d, message=%s\n",
                  errorDetails.code, errorDetails.message.c_str());

    // Checking standard error codes
    if (errorDetails.code == JSONRPC_METHOD_NOT_FOUND) {
      Serial.println("Action: Method not implemented on the server.");
    } else if (errorDetails.code == JSONRPC_INVALID_PARAMS) {
      Serial.println("Action: Please check the method parameters.");
    } else {
      Serial.printf("Action: General server error (%d)\n", errorDetails.code);
    }
  }

  delay(15000);
}
