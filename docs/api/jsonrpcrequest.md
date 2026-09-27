---
title: JsonRpcRequest Class Reference - Fluent JSON-RPC 2.0 Client for ESP32
description: API reference for JsonRpcRequest class: method chaining for JSON-RPC 2.0 requests, positional and named parameters, request IDs, notifications, and error handling.
keywords: JsonRpcRequest class ESP32, JSON-RPC 2.0 client ESP32, JSON-RPC HTTP, JSON-RPC notifications, JSON-RPC error handling
tags:
  - api
  - jsonrpc
  - request
  - class
---
# JsonRpcRequest

The fluent request builder returned by `.jsonRpc(path)` and `.jsonrpc(path)` on [`ESP32HTTPClient`](esp32httpclient.md). All builder methods return `JsonRpcRequest&`, enabling clean fluent chaining.

**The JSON-RPC request is dispatched automatically** when the `JsonRpcRequest` object goes out of scope (at the end of the statement) or when `.execute()` is called explicitly.

!!! note "Copy semantics"
    `JsonRpcRequest` is **move-only** — it cannot be copied. It is designed to be used in a single chained expression or moved safely.

---

## Method Chaining Overview

```cpp
ESP32HTTPClient client("https://api.example.com");

int sum = 0;
client.jsonRpc("/rpc")
    .method("add")
    .param(15)
    .param(27)
    .id(1)
    .getResult(&sum);
```

---

## Configuring the Method & ID

### `method(name)`
Sets the remote method name to invoke.
```cpp
JsonRpcRequest& method(const char* methodName);
JsonRpcRequest& method(const String& methodName);
```

### `id(reqId)`
Sets a configurable request ID (integer or string).
```cpp
JsonRpcRequest& id(int reqId);
JsonRpcRequest& id(long reqId);
JsonRpcRequest& id(const char* reqId);
JsonRpcRequest& id(const String& reqId);
```

### `nullId()`
Explicitly sets the `"id"` field to `null`.
```cpp
JsonRpcRequest& nullId();
```

### `notification()` / `asNotification()`
Configures the request as a JSON-RPC 2.0 Notification. The `"id"` field is completely omitted from the request payload according to the JSON-RPC 2.0 specification, and no response body is expected from the server.
```cpp
JsonRpcRequest& notification(bool enable = true);
JsonRpcRequest& asNotification();
```

---

## Parameters

Supports both positional (array) and named (object) parameters, raw JSON, and C++ structs decorated with `REST_JSON_MAP`.

### Positional Parameters (Array)
Calling `.param(val)` or `.positionalParam(val)` adds values to a JSON array parameter list:
```cpp
client.jsonRpc("/rpc")
    .method("subtract")
    .param(42)
    .param(23);
// JSON payload: {"jsonrpc":"2.0","method":"subtract","params":[42,23],"id":1}
```

### Named Parameters (Object)
Calling `.param(name, val)` or `.namedParam(name, val)` constructs a key-value JSON object:
```cpp
client.jsonRpc("/rpc")
    .method("subtract")
    .param("minuend", 42)
    .param("subtrahend", 23);
// JSON payload: {"jsonrpc":"2.0","method":"subtract","params":{"minuend":42,"subtrahend":23},"id":1}
```

Supported parameter types:
* `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`
* `float`, `double`
* `bool`
* `const char*`, `String`

### Struct Parameters
C++ structs decorated with `REST_JSON_MAP` can be passed directly as the parameters object or as a named parameter:
```cpp
struct SensorData {
  float temp;
  int humidity;
  REST_JSON_MAP(REST_FIELD(temp), REST_FIELD(humidity))
};

SensorData data{24.5f, 60};
client.jsonRpc("/rpc").method("report").params(data);
```

### Raw JSON Parameters
For pre-formatted JSON parameters:
```cpp
client.jsonRpc("/rpc")
    .method("complex")
    .rawParams("[{\"nested\": true}]");
```

---

## Binding and Parsing Results

Results can be bound directly to variables, streamed without buffering the whole response in heap memory.

### Primitive Result Binding
```cpp
int result = 0;
client.jsonRpc("/rpc").method("add").param(10).param(5).getResult(&result);
```

### Object Subpath Binding
When the result is a JSON object, extract subfields by path:
```cpp
float temp = 0.0f;
String unit = "";
client.jsonRpc("/rpc")
    .method("getWeather")
    .getResult("temperature", &temp)
    .getResult("unit", &unit);
```

### Struct Result Binding
Directly deserialize the result object into a struct:
```cpp
struct Weather {
  float temperature;
  String unit;
  REST_JSON_MAP(REST_FIELD(temperature), REST_FIELD(unit))
};

Weather w;
client.jsonRpc("/rpc").method("getWeather").getResult(&w);
```

### Array Element Binding
```cpp
int first = 0, second = 0;
client.jsonRpc("/rpc")
    .method("getNumbers")
    .getResult("[0]", &first)
    .getResult("[1]", &second);
```

### Raw Result and Response
```cpp
String rawResult;
String rawResponse;
client.jsonRpc("/rpc")
    .method("getData")
    .getRawResult(&rawResult)
    .getRawResponse(&rawResponse);
```

---

## Error Handling

JSON-RPC 2.0 error responses (`"error": {"code": -32601, "message": "...", "data": "..."}`) are automatically parsed and exposed.

### Callback
```cpp
client.jsonRpc("/rpc")
    .method("invalidMethod")
    .onJsonRpcError([](const JsonRpcError& err) {
      Serial.printf("JSON-RPC Error %d: %s\n", err.code, err.message.c_str());
      if (!err.data.isEmpty()) {
        Serial.printf("Details: %s\n", err.data.c_str());
      }
    });
```

### Direct Inspection
```cpp
JsonRpcError err;
auto req = client.jsonRpc("/rpc");
req.method("compute").getError(&err);
req.execute();

if (req.hasJsonRpcError()) {
  Serial.printf("Code: %d, Message: %s\n", err.code, err.message.c_str());
}
```

### Standard Error Constants
| Constant | Value | Description |
|---|---|---|
| `JSONRPC_PARSE_ERROR` | `-32700` | Invalid JSON received by server |
| `JSONRPC_INVALID_REQUEST` | `-32600` | Payload is not a valid JSON-RPC 2.0 request |
| `JSONRPC_METHOD_NOT_FOUND` | `-32601` | Method does not exist |
| `JSONRPC_INVALID_PARAMS` | `-32602` | Invalid parameters |
| `JSONRPC_INTERNAL_ERROR` | `-32603` | Internal JSON-RPC server error |
| `JSONRPC_SERVER_ERROR_START` | `-32099` | Server-reserved error range start |
| `JSONRPC_SERVER_ERROR_END` | `-32000` | Server-reserved error range end |

---

## HTTP Configuration

* `header(name, value)`: Adds a custom HTTP header for this request.
* `contentType(type)`: Overrides the default `application/json` Content-Type.
* `timeout(ms)`: Sets a custom request timeout.
* `maxRetry(count)`: Sets max retry attempts for connection failures.
* `path(param, value)`: Resolves `{param}` in the URL path.
* `queryParam(key, value)`: Appends query parameters to the URL.
