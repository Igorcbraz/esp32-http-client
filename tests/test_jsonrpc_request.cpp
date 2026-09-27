#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "HTTPClient.h"

#define private public
#include "ESP32HTTPClient.h"
#include "JsonRpcRequest.h"
#include "JsonRpcTypes.h"
#undef private

namespace {
int failures = 0;
int checks = 0;
int passedChecks = 0;
int suitesRun = 0;
int suitesPassed = 0;

void expectTrue(bool condition, const char* message) {
  checks++;
  if (!condition) {
    std::cerr << "[FAIL] " << message << "\n";
    failures++;
    return;
  }
  passedChecks++;
}

void expectEq(const std::string& actual, const std::string& expected, const char* message) {
  checks++;
  if (actual != expected) {
    std::cerr << "[FAIL] " << message << " (expected: " << expected << ", got: " << actual << ")\n";
    failures++;
    return;
  }
  passedChecks++;
}

void expectEqInt(long long actual, long long expected, const char* message) {
  checks++;
  if (actual != expected) {
    std::cerr << "[FAIL] " << message << " (expected: " << expected << ", got: " << actual << ")\n";
    failures++;
    return;
  }
  passedChecks++;
}

void expectNear(double actual, double expected, double tolerance, const char* message) {
  checks++;
  if (std::fabs(actual - expected) > tolerance) {
    std::cerr << "[FAIL] " << message << " (expected: " << expected << ", got: " << actual << ")\n";
    failures++;
    return;
  }
  passedChecks++;
}

void expectContains(const std::string& haystack, const std::string& needle, const char* message) {
  checks++;
  if (haystack.find(needle) == std::string::npos) {
    std::cerr << "[FAIL] " << message << " (expected to contain: " << needle << ", got: " << haystack << ")\n";
    failures++;
    return;
  }
  passedChecks++;
}

void runSuite(const char* name, void (*fn)()) {
  suitesRun++;
  int before = failures;
  std::cout << "[RUN ] " << name << "\n";
  fn();
  if (failures == before) {
    suitesPassed++;
    std::cout << "[PASS] " << name << "\n";
  } else {
    std::cout << "[FAIL] " << name << " (" << (failures - before) << " failure(s))\n";
  }
}

// Struct for struct mapping tests
struct DeviceStatus {
  int battery;
  float temperature;
  bool online;
  String firmware;

  REST_JSON_MAP(
    REST_FIELD(battery),
    REST_FIELD(temperature),
    REST_FIELD(online),
    REST_FIELD(firmware)
  )
};

// 1. Basic Request Formatting
void testBasicRequestFormatting() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":\"ok\",\"id\":1}");

  ESP32HTTPClient client("http://example.com");
  auto req = client.jsonRpc("/rpc");
  req.method("ping");
  req.id(1);

  String payload = req.buildRequestBody();
  expectContains(payload.c_str(), "\"jsonrpc\":\"2.0\"", "Should specify jsonrpc 2.0");
  expectContains(payload.c_str(), "\"method\":\"ping\"", "Should have ping method");
  expectContains(payload.c_str(), "\"id\":1", "Should have id 1");

  req.execute();
  expectEq(HttpClientStub::lastUrl, "http://example.com/rpc", "URL matches");
  expectContains(HttpClientStub::lastPayload, "\"method\":\"ping\"", "Payload was sent");
}

// 2. Positional Parameters
void testPositionalParameters() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":19,\"id\":1}");

  ESP32HTTPClient client("http://example.com");
  auto req = client.jsonRpc("/rpc");
  req.method("subtract")
     .id(1)
     .param(42)
     .param(23);

  String payload = req.buildRequestBody();
  expectContains(payload.c_str(), "\"params\":[42,23]", "Positional params serialized as array");

  // Diverse positional types
  auto req2 = client.jsonRpc("/rpc");
  req2.method("testTypes")
      .id(2)
      .positionalParam("hello")
      .positionalParam(true)
      .positionalParam((long)123456789)
      .positionalParam(3.14f);

  String p2 = req2.buildRequestBody();
  expectContains(p2.c_str(), "\"params\":[\"hello\",true,123456789,3.14", "Positional types formatted properly");
}

// 3. Named Parameters
void testNamedParameters() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":19,\"id\":1}");

  ESP32HTTPClient client("http://example.com");
  auto req = client.jsonRpc("/rpc");
  req.method("subtract")
     .id(1)
     .param("subtrahend", 23)
     .param("minuend", 42);

  String payload = req.buildRequestBody();
  expectContains(payload.c_str(), "\"params\":{", "Named params serialized as object");
  expectContains(payload.c_str(), "\"subtrahend\":23", "subtrahend in object");
  expectContains(payload.c_str(), "\"minuend\":42", "minuend in object");

  // Named param aliases and types
  auto req2 = client.jsonRpc("/rpc");
  req2.method("config")
      .id(2)
      .namedParam("host", "iot.local")
      .namedParam("enabled", false)
      .namedParam("port", 8080);

  String p2 = req2.buildRequestBody();
  expectContains(p2.c_str(), "\"host\":\"iot.local\"", "string named param");
  expectContains(p2.c_str(), "\"enabled\":false", "bool named param");
  expectContains(p2.c_str(), "\"port\":8080", "numeric named param");
}

// 4. Configurable IDs
void testConfigurableIds() {
  ESP32HTTPClient client("http://example.com");

  // Numeric ID
  auto req1 = client.jsonRpc();
  req1.method("test").id(999);
  expectContains(req1.buildRequestBody().c_str(), "\"id\":999", "Int ID");
  expectEqInt(req1.getRequestIdInt(), 999, "getRequestIdInt matches");
  expectEq(req1.getRequestId().c_str(), "999", "getRequestId matches");

  // String ID
  auto req2 = client.jsonRpc();
  req2.method("test").id("uuid-1234");
  expectContains(req2.buildRequestBody().c_str(), "\"id\":\"uuid-1234\"", "String ID");
  expectEq(req2.getRequestId().c_str(), "uuid-1234", "String getRequestId");

  // Null ID
  auto req3 = client.jsonRpc();
  req3.method("test").nullId();
  expectContains(req3.buildRequestBody().c_str(), "\"id\":null", "Null ID");
  expectEq(req3.getRequestId().c_str(), "null", "Null getRequestId");

  // Auto-assigned ID
  auto req4 = client.jsonRpc();
  req4.method("test");
  expectTrue(req4.hasRequestId(), "Auto ID has request ID");
  expectTrue(req4.getRequestIdInt() > 0, "Auto ID generates positive int");
}

// 5. Notifications
void testNotifications() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(204, ""); // 204 No Content

  ESP32HTTPClient client("http://example.com");
  auto req = client.jsonRpc("/rpc");
  req.method("updateStatus")
     .asNotification()
     .param("status", "idle");

  expectTrue(req.isNotification(), "Request is marked as notification");
  String payload = req.buildRequestBody();
  expectContains(payload.c_str(), "\"method\":\"updateStatus\"", "Method present");
  expectTrue(payload.indexOf("\"id\"") == -1, "ID MUST NOT be present in notification");

  bool successFired = false;
  req.onSuccess([&](int code) {
    successFired = true;
    expectEqInt(code, 204, "HTTP 204 received");
  });

  req.execute();
  expectTrue(successFired, "Notification succeeds on HTTP 204");
  expectTrue(req.isSuccess(), "isSuccess is true");
  expectTrue(!req.hasError(), "hasError is false");
}

// 6. Primitive Response Parsing
void testPrimitiveResponseParsing() {
  HttpClientStub::reset();

  // Int result
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":42,\"id\":1}");
  ESP32HTTPClient client("http://example.com");
  int intRes = 0;
  client.jsonRpc("/rpc").method("getInt").id(1).getResult(&intRes);
  expectEqInt(intRes, 42, "Parsed int result");

  // String result
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":\"hello world\",\"id\":2}");
  String strRes;
  char bufRes[32] = {0};
  client.jsonRpc("/rpc").method("getStr").id(2).getResult(&strRes).getResult(bufRes);
  expectEq(strRes.c_str(), "hello world", "Parsed Arduino String result");
  expectEq(bufRes, "hello world", "Parsed char buffer result");

  // Boolean result
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":true,\"id\":3}");
  bool boolRes = false;
  client.jsonRpc("/rpc").method("getBool").id(3).getResult(&boolRes);
  expectTrue(boolRes, "Parsed boolean true result");

  // Float & Double result
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":3.14159,\"id\":4}");
  float floatRes = 0.0f;
  double doubleRes = 0.0;
  client.jsonRpc("/rpc").method("getNum").id(4).getResult(&floatRes).getResult(&doubleRes);
  expectNear(floatRes, 3.14159f, 0.001, "Parsed float result");
  expectNear(doubleRes, 3.14159, 0.0001, "Parsed double result");

  // Long result
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":2147483647,\"id\":5}");
  long longRes = 0;
  client.jsonRpc("/rpc").method("getLong").id(5).getResult(&longRes);
  expectEqInt(longRes, 2147483647L, "Parsed long result");
}

// 7. Object Response Parsing
void testObjectResponseParsing() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":{\"temperature\":24.5,\"unit\":\"C\",\"sensor\":{\"id\":\"s1\",\"active\":true}},\"id\":1}");

  ESP32HTTPClient client("http://example.com");
  float temp = 0.0f;
  String unit = "";
  String sensorId = "";
  bool active = false;

  auto req = client.jsonRpc("/rpc");
  req.method("getSensorData").id(1)
     .getResult("temperature", &temp)
     .getResult("unit", &unit)
     .getResult("sensor.id", &sensorId)
     .getResult("sensor.active", &active);
  req.execute();

  expectNear(temp, 24.5f, 0.01, "temperature field parsed");
  expectEq(unit.c_str(), "C", "unit field parsed");
  expectEq(sensorId.c_str(), "s1", "sensor.id nested field parsed");
  expectTrue(active, "sensor.active nested boolean parsed");
}

// 8. Array Response Parsing
void testArrayResponseParsing() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":[100, 200, 300],\"id\":1}");

  ESP32HTTPClient client("http://example.com");
  int a = 0, b = 0, c = 0;
  client.jsonRpc("/rpc").method("getPoints").id(1)
        .getResult("[0]", &a)
        .getResult("[1]", &b)
        .getResult("[2]", &c);

  expectEqInt(a, 100, "Array element 0");
  expectEqInt(b, 200, "Array element 1");
  expectEqInt(c, 300, "Array element 2");
}

// 9. Struct Serialization & Deserialization
void testStructSerializationAndDeserialization() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":{\"battery\":92,\"temperature\":22.8,\"online\":true,\"firmware\":\"v1.2.0\"},\"id\":10}");

  ESP32HTTPClient client("http://example.com");

  // Send struct as param
  DeviceStatus inputStatus{85, 25.1f, true, "v1.1.0"};
  auto req = client.jsonRpc("/rpc");
  req.method("syncStatus").id(10).params(inputStatus);

  String payload = req.buildRequestBody();
  expectContains(payload.c_str(), "\"battery\":85", "Struct serialized battery");
  expectContains(payload.c_str(), "\"online\":true", "Struct serialized online");
  expectContains(payload.c_str(), "\"firmware\":\"v1.1.0\"", "Struct serialized firmware");

  // Bind struct to result
  DeviceStatus outputStatus{0, 0.0f, false, ""};
  req.getResult(&outputStatus);
  req.execute();

  expectEqInt(outputStatus.battery, 92, "Parsed struct battery");
  expectNear(outputStatus.temperature, 22.8f, 0.01, "Parsed struct temperature");
  expectTrue(outputStatus.online, "Parsed struct online");
  expectEq(outputStatus.firmware.c_str(), "v1.2.0", "Parsed struct firmware");
}

// 10. Raw Result and Raw Response
void testRawResultAndRawResponse() {
  HttpClientStub::reset();
  std::string fullResponse = "{\"jsonrpc\":\"2.0\",\"result\":{\"nested\":{\"value\":123}},\"id\":1}";
  HttpClientStub::setResponse(200, fullResponse);

  ESP32HTTPClient client("http://example.com");
  String rawRes;
  String rawResp;
  client.jsonRpc("/rpc").method("getData").id(1)
        .getRawResult(&rawRes)
        .getRawResponse(&rawResp);

  expectContains(rawRes.c_str(), "\"nested\":{\"value\":123}", "Captured raw result");
  expectEq(rawResp.c_str(), fullResponse, "Captured entire raw response");
}

// 11. Error Handling and Callbacks
void testErrorHandling() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32601,\"message\":\"Method not found\",\"data\":\"custom error details\"},\"id\":1}");

  ESP32HTTPClient client("http://example.com");
  auto req = client.jsonRpc("/rpc");
  req.method("unknownMethod").id(1);

  bool jsonRpcErrorCallbackCalled = false;
  JsonRpcError capturedError;
  req.onJsonRpcError([&](const JsonRpcError& err) {
    jsonRpcErrorCallbackCalled = true;
    capturedError = err;
  });

  bool errorCallbackCalled = false;
  req.onError([&](int code, const char* msg) {
    errorCallbackCalled = true;
    expectEqInt(code, -32601, "Error code in onError");
    expectEq(msg, "Method not found", "Error message in onError");
  });

  JsonRpcError directError;
  int errCode = 0;
  String errMsg = "";
  String errData = "";
  req.getError(&directError);
  req.getErrorCode(&errCode);
  req.getErrorMessage(&errMsg);
  req.getErrorData(&errData);

  req.execute();

  expectTrue(req.hasError(), "hasError() is true");
  expectTrue(req.hasJsonRpcError(), "hasJsonRpcError() is true");
  expectTrue(!req.isSuccess(), "isSuccess() is false");
  expectTrue(jsonRpcErrorCallbackCalled, "onJsonRpcError callback fired");
  expectTrue(errorCallbackCalled, "onError callback fired");

  expectEqInt(capturedError.code, -32601, "Error code matched");
  expectEq(capturedError.message.c_str(), "Method not found", "Error message matched");
  expectEq(capturedError.data.c_str(), "custom error details", "Error data matched");

  expectEqInt(errCode, -32601, "Direct error code");
  expectEq(errMsg.c_str(), "Method not found", "Direct error message");
  expectEq(errData.c_str(), "custom error details", "Direct error data");
  expectEqInt(directError.code, -32601, "Direct struct error code");
}

// 12. Standard Error Codes
void testStandardErrorCodes() {
  expectEqInt(JSONRPC_PARSE_ERROR, -32700, "Parse error code");
  expectEqInt(JSONRPC_INVALID_REQUEST, -32600, "Invalid request code");
  expectEqInt(JSONRPC_METHOD_NOT_FOUND, -32601, "Method not found code");
  expectEqInt(JSONRPC_INVALID_PARAMS, -32602, "Invalid params code");
  expectEqInt(JSONRPC_INTERNAL_ERROR, -32603, "Internal error code");
  expectEqInt(JSONRPC_SERVER_ERROR_START, -32099, "Server error start");
  expectEqInt(JSONRPC_SERVER_ERROR_END, -32000, "Server error end");
}

// 13. Headers and Authentication
void testHeadersAndAuth() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":\"ok\",\"id\":1}");
  std::vector<std::pair<std::string, std::string>> respHeaders = {
    {"X-Rate-Limit", "100"},
    {"Server", "TestRPC/1.0"}
  };
  HttpClientStub::setResponseHeaders(respHeaders);

  ESP32HTTPClient client("http://example.com");
  client.bearer("my-secret-jwt-token");
  client.cookie("session_id", "sess9876");

  int rateLimit = 0;
  String serverHeader = "";

  auto req = client.jsonRpc("/rpc");
  req.method("getData").id(1)
     .header("X-Client-App", "ESP32-Device")
     .getHeader("X-Rate-Limit", &rateLimit)
     .getHeader("Server", &serverHeader);
  req.execute();

  bool foundAuth = false;
  bool foundCookie = false;
  bool foundCustom = false;
  bool foundContentType = false;

  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Authorization" && h.second == "Bearer my-secret-jwt-token") foundAuth = true;
    if (h.first == "Cookie" && h.second == "session_id=sess9876") foundCookie = true;
    if (h.first == "X-Client-App" && h.second == "ESP32-Device") foundCustom = true;
    if (h.first == "Content-Type" && h.second == "application/json") foundContentType = true;
  }

  expectTrue(foundAuth, "Bearer auth header sent");
  expectTrue(foundCookie, "Cookie sent");
  expectTrue(foundCustom, "Custom header sent");
  expectTrue(foundContentType, "Content-Type application/json sent");
  expectEqInt(rateLimit, 100, "Read response header X-Rate-Limit");
  expectEq(serverHeader.c_str(), "TestRPC/1.0", "Read response header Server");
}

// 14. URL Path Parameters & Query Parameters
void testUrlPathAndQueryParams() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":\"ok\",\"id\":1}");

  ESP32HTTPClient client("http://example.com");
  client.jsonRpc("/api/{version}/rpc")
        .path("version", "v2")
        .queryParam("apiKey", "key_abc")
        .method("ping").id(1);

  expectEq(HttpClientStub::lastUrl, "http://example.com/api/v2/rpc?apiKey=key_abc", "URL templating and query param");
}

// 15. Retry and Observability
void testRetryAndObservability() {
  HttpClientStub::reset();
  // First attempt fails, second succeeds
  HttpClientStub::queueResponse(-1, "");
  HttpClientStub::queueResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":\"success\",\"id\":1}");

  ESP32HTTPClient client("http://example.com");
  client.setMaxRetry(2);

  bool obsFired = false;
  client.onObservability([&](const ObservabilityMetrics& m) {
    obsFired = true;
    expectEqInt(m.retries, 1, "One retry executed");
    expectTrue(m.txBytes > 0, "txBytes tracked");
  });

  String result;
  client.jsonRpc("/rpc").method("testRetry").id(1).getResult(&result);

  expectTrue(obsFired, "Observability callback fired");
  expectEq(result.c_str(), "success", "Result captured after retry");
}

// 16. Streaming Large Response
void testStreamingLargeResponse() {
  HttpClientStub::reset();
  std::string largeJson = "{\"jsonrpc\":\"2.0\",\"result\":{\"field_a\":\"";
  largeJson.append(2000, 'X');
  largeJson += "\",\"target_num\":777},\"id\":1}";
  HttpClientStub::setResponse(200, largeJson);

  ESP32HTTPClient client("http://example.com");
  int num = 0;
  // Stream directly and bind target_num without buffering field_a
  client.jsonRpc("/rpc").method("large").id(1).getResult("target_num", &num);

  expectEqInt(num, 777, "Stream-parsed field from large response");
}

// 17. Move Constructor Semantics
void testMoveConstructor() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":555,\"id\":1}");

  ESP32HTTPClient client("http://example.com");
  int target = 0;
  {
    auto req1 = client.jsonRpc("/rpc");
    req1.method("testMove").id(1).getResult(&target);
    JsonRpcRequest req2(std::move(req1));
    expectTrue(req1._executed, "Original request marked executed after move");
    expectTrue(!req2._executed, "Moved request not yet executed");
  } // req2 destructor executes HTTP call!

  expectEqInt(target, 555, "Result parsed via moved request execution");
}

// 18. Client Method Aliases and ID/Version Getters
void testClientMethods() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":\"pong\",\"id\":88}");

  ESP32HTTPClient client("http://example.com");

  String version = "";
  int respId = 0;
  String resultStr = "";

  // Test client.jsonRpc and client.jsonrpc
  auto req = client.jsonrpc("/rpc");
  req.method("ping").id(88)
     .getResult(&resultStr)
     .getId(&respId)
     .getJsonRpcVersion(&version);
  req.execute();

  expectEq(resultStr.c_str(), "pong", "Result pong");
  expectEqInt(respId, 88, "Response ID 88");
  expectEq(version.c_str(), "2.0", "JSON-RPC Version 2.0");
  expectEq(req.getResponseId().c_str(), "88", "getResponseId");
  expectEq(req.getJsonRpcVersion().c_str(), "2.0", "getJsonRpcVersion");
}

// 19. All Parameter Overloads and Request Builder Setters
void testParamOverloadsAndBuilderSetters() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":true,\"id\":100}");

  ESP32HTTPClient client("http://example.com");

  // Positional parameters with all scalar types
  {
    auto req = client.jsonRpc("/rpc");
    String mName = "allPositional";
    req.method(mName);
    expectEq(req.getMethod().c_str(), "allPositional", "getMethod()");

    String strParam = "hello";
    unsigned int uIntVal = 123u;
    unsigned long uLongVal = 456ul;
    long long lLongVal = 789ll;
    unsigned long long ulLongVal = 999ull;
    double dVal = 3.14159;
    float fVal = 2.718f;

    req.param(strParam)
       .param(uIntVal)
       .param(uLongVal)
       .param(lLongVal)
       .param(ulLongVal)
       .param(fVal)
       .param(dVal)
       .rawParam("{\"nested\":1}")
       .rawParam(String("[1,2,3]"))
       .id((long)100);

    req.contentType("application/json-rpc")
       .timeout(2500)
       .maxRetry(2)
       .retry(3);

    req.execute();

    expectContains(HttpClientStub::lastPayload, "\"method\":\"allPositional\"", "Payload contains method");
    expectContains(HttpClientStub::lastPayload, "\"hello\"", "Payload contains string param");
    expectContains(HttpClientStub::lastPayload, "123", "Payload contains uint param");
    expectContains(HttpClientStub::lastPayload, "456", "Payload contains ulong param");
    expectContains(HttpClientStub::lastPayload, "789", "Payload contains llong param");
    expectContains(HttpClientStub::lastPayload, "999", "Payload contains ullong param");
    expectContains(HttpClientStub::lastPayload, "{\"nested\":1}", "Payload contains rawParam");
    expectContains(HttpClientStub::lastPayload, "[1,2,3]", "Payload contains rawParam String");
  }

  // Named parameters with all scalar types
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":true,\"id\":\"req-str\"}");
  {
    auto req = client.jsonRpc("/rpc");
    req.method("allNamed");
    String sName = "strK";
    String sVal = "strV";
    unsigned int uIntVal = 10u;
    long lVal = 20l;
    unsigned long ulVal = 30ul;
    long long llVal = 40ll;
    unsigned long long ullVal = 50ull;
    float fVal = 1.5f;
    double dVal = 2.5;

    req.param(sName.c_str(), sVal)
       .param("uIntK", uIntVal)
       .param("lK", lVal)
       .param("ulK", ulVal)
       .param("llK", llVal)
       .param("ullK", ullVal)
       .param("fK", fVal)
       .param("dK", dVal)
       .rawParam("rawK1", "{\"x\":1}")
       .rawParam("rawK2", String("[true]"))
       .id(String("req-str"));

    req.execute();

    expectContains(HttpClientStub::lastPayload, "\"strK\":\"strV\"", "Named string param");
    expectContains(HttpClientStub::lastPayload, "\"uIntK\":10", "Named uint param");
    expectContains(HttpClientStub::lastPayload, "\"lK\":20", "Named long param");
    expectContains(HttpClientStub::lastPayload, "\"ulK\":30", "Named ulong param");
    expectContains(HttpClientStub::lastPayload, "\"llK\":40", "Named llong param");
    expectContains(HttpClientStub::lastPayload, "\"ullK\":50", "Named ullong param");
    expectContains(HttpClientStub::lastPayload, "\"rawK1\":{\"x\":1}", "Named rawParam");
    expectContains(HttpClientStub::lastPayload, "\"rawK2\":[true]", "Named rawParam String");
    expectContains(HttpClientStub::lastPayload, "\"id\":\"req-str\"", "String ID");
  }

  // rawParams (object and array) and notification toggling
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":null}");
  {
    auto req = client.jsonRpc("/rpc");
    req.method("rawTest")
       .rawParams("{\"custom\":\"param\"}")
       .notification(true);

    req.execute();
    expectContains(HttpClientStub::lastPayload, "\"params\":{\"custom\":\"param\"}", "rawParams object");
    expectTrue(HttpClientStub::lastPayload.find("\"id\":") == std::string::npos, "No ID for notification(true)");
  }

  // String escaping in params: \b, \f, \n, \r, \t, \", \\, \u001f
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":true,\"id\":1}");
  {
    String escInput = "a\"b\\c\bd\fe\nf\rg\th\x1f";
    client.jsonRpc("/rpc")
          .method("escTest")
          .param(escInput)
          .id(1)
          .execute();

    expectContains(HttpClientStub::lastPayload, "\\\"b\\\\c\\bd\\fe\\nf\\rg\\th\\u001f", "Escaped characters in params");
  }
}

// 20. Response Bindings for All Types and Headers
void testResponseBindingsAndHeadersAllTypes() {
  HttpClientStub::reset();
  HttpClientStub::setResponseHeaders({
    {"X-Custom-Float", "12.34"},
    {"X-Custom-Double", "56.78"},
    {"X-Custom-Bool", "true"},
    {"X-Custom-Long", "987654"},
    {"X-Custom-Char", "abcde"}
  });
  HttpClientStub::setResponse(200, 
    "{\"jsonrpc\":\"2.0\",\"result\":{"
      "\"lVal\":123456789,"
      "\"iVal\":98765,"
      "\"dVal\":45.67,"
      "\"cStr\":\"my-result\","
      "\"bVal\":true"
    "},\"id\":99999999}");

  ESP32HTTPClient client("http://example.com");

  long lRes = 0;
  int iRes = 0;
  double dRes = 0.0;
  char cRes[32] = {0};
  long rpcIdLong = 0;
  String rpcIdStr = "";

  float hFloat = 0.0f;
  double hDouble = 0.0;
  bool hBool = false;
  long hLong = 0;
  char hChar[16] = {0};

  auto req = client.jsonRpc("/rpc");
  req.method("testTypes")
     .getResult("lVal", &lRes)
     .getResult("iVal", &iRes)
     .getResult("dVal", &dRes)
     .getResult("cStr", cRes, sizeof(cRes))
     .getId(&rpcIdLong)
     .getId(&rpcIdStr)
     .getHeader("X-Custom-Float", &hFloat)
     .getHeader("X-Custom-Double", &hDouble)
     .getHeader("X-Custom-Bool", &hBool)
     .getHeader("X-Custom-Long", &hLong)
     .getHeader("X-Custom-Char", hChar, sizeof(hChar))
     .execute();

  expectEqInt(lRes, 123456789, "Result long");
  expectEqInt(iRes, 98765, "Result int");
  expectNear(dRes, 45.67, 0.01, "Result double");
  expectEq(cRes, "my-result", "Result char array");
  expectEqInt(rpcIdLong, 99999999l, "getId(long*)");
  expectEq(rpcIdStr.c_str(), "99999999", "getId(String*)");
  expectEqInt(req.getResponseIdInt(), 99999999, "getResponseIdInt()");

  expectNear(hFloat, 12.34f, 0.01, "Header float");
  expectNear(hDouble, 56.78, 0.01, "Header double");
  expectTrue(hBool, "Header bool");
  expectEqInt(hLong, 987654l, "Header long");
  expectEq(hChar, "abcde", "Header char array");
  expectEqInt(req.getStatusCode(), 200, "getStatusCode()");

  // Direct root getResult for scalar types
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":1234567,\"id\":1}");
  long rootLong = 0;
  client.jsonRpc("/rpc").method("test").getResult(&rootLong).execute();
  expectEqInt(rootLong, 1234567, "Root long result");

  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":true,\"id\":1}");
  bool rootBool = false;
  client.jsonRpc("/rpc").method("test").getResult(&rootBool).execute();
  expectTrue(rootBool, "Root bool result");

  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":12.3456,\"id\":1}");
  double rootDouble = 0.0;
  client.jsonRpc("/rpc").method("test").getResult(&rootDouble).execute();
  expectNear(rootDouble, 12.3456, 0.001, "Root double result");
}

// 21. Callbacks, Error Details, and Client Integration
void testCallbacksAndErrorDetails() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32601,\"message\":\"Method not found\",\"data\":\"detailed context\"},\"id\":5}");

  ESP32HTTPClient client("http://example.com");

  bool clientSuccessCalled = false;
  bool clientErrorCalled = false;
  bool clientResponseCalled = false;
  client.onSuccess([&](int) { clientSuccessCalled = true; });
  client.onError([&](int, const char*) { clientErrorCalled = true; });
  client.onResponse([&](int) { clientResponseCalled = true; });

  bool singleParamErrorCalled = false;
  bool reqResponseCalled = false;
  bool rpcErrorCalled = false;
  JsonRpcError caughtRpcErr;

  auto req = client.jsonRpc("/rpc");
  req.method("nonExistent")
     .id(5)
     .onError([&](int) { singleParamErrorCalled = true; })
     .onResponse([&](int) { reqResponseCalled = true; })
     .onJsonRpcError([&](const JsonRpcError& err) {
        rpcErrorCalled = true;
        caughtRpcErr = err;
     })
     .execute();

  expectTrue(singleParamErrorCalled, "Single-param onError called on JSON-RPC error");
  expectTrue(reqResponseCalled, "onResponse called on JSON-RPC error");
  expectTrue(rpcErrorCalled, "onJsonRpcError called");
  expectEqInt(caughtRpcErr.code, -32601, "Caught rpc error code");
  expectEq(caughtRpcErr.message.c_str(), "Method not found", "Caught rpc error message");
  expectEq(caughtRpcErr.data.c_str(), "detailed context", "Caught rpc error data");

  expectEqInt(req.getErrorCode(), -32601, "getErrorCode()");
  expectEq(req.getErrorMessage().c_str(), "Method not found", "getErrorMessage()");
  expectEq(req.getErrorData().c_str(), "detailed context", "getErrorData()");
  expectEqInt(req.getError().code, -32601, "getError().code");

  // Client-level onError clears
  req.onError((HttpResponseCallback)nullptr);

  // Client-level callbacks on success
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":\"ok\",\"id\":1}");
  clientSuccessCalled = false;
  client.jsonRpc("/rpc").method("test").id(1).execute();
  expectTrue(clientSuccessCalled, "Client onSuccess called on JSON-RPC success");

  // Client-level callbacks on network HTTP failure
  HttpClientStub::reset();
  HttpClientStub::setResponse(-1, "");
  clientErrorCalled = false;
  bool reqHttpErrorCalled = false;
  client.jsonRpc("/rpc")
        .method("test")
        .onError([&](int, const char*) { reqHttpErrorCalled = true; })
        .execute();
  expectTrue(reqHttpErrorCalled, "Request onError called on network failure");
}

// 22. Edge Cases: Custom Port, Auto-Execution, Parsing Anomalies
void testEdgeCasesAndParsingAnomalies() {
  // Custom port with subpath in client
  {
    ESP32HTTPClient clientWithPort("http://example.com/api/v1", 8080);
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":true,\"id\":1}");
    clientWithPort.jsonRpc("/rpc").method("ping").id(1).execute();
    expectContains(HttpClientStub::lastUrl, ":8080/api/v1/rpc", "Custom port and subpath in URL");
  }

  // Destructor auto-execution
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":\"auto\",\"id\":7}");
    ESP32HTTPClient client("http://example.com");
    {
      auto r = client.jsonRpc("/auto");
      r.method("autoExec").id(7);
    }
    expectContains(HttpClientStub::lastUrl, "/auto", "Auto-executed in destructor");
  }

  // Negative and string IDs in responses
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":10,\"id\":-42}");
    ESP32HTTPClient client("http://example.com");
    long negId = 0;
    client.jsonRpc("/rpc").method("test").getId(&negId).execute();
    expectEqInt(negId, -42, "Negative response ID parsed");
  }

  // Null id in response
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":20,\"id\":null}");
    ESP32HTTPClient client("http://example.com");
    String nullIdStr = "";
    auto req = client.jsonRpc("/rpc");
    req.method("test").getId(&nullIdStr).execute();
    expectEq(nullIdStr.c_str(), "null", "Null response ID parsed as 'null'");
  }

  // Malformed JSON responses
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "not-a-json-object");
    ESP32HTTPClient client("http://example.com");
    bool errCalled = false;
    client.jsonRpc("/rpc").method("test").onError([&](int, const char*) { errCalled = true; }).execute();
    expectTrue(errCalled, "Malformed JSON triggers error callback");
  }

  // Missing result and error in JSON response
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"id\":1}");
    ESP32HTTPClient client("http://example.com");
    auto req = client.jsonRpc("/rpc");
    req.method("test").id(1).execute();
    expectTrue(!req.hasJsonRpcError(), "No error when error object is absent");
  }

  // Primitive error data (number, boolean)
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32000,\"message\":\"Server error\",\"data\":12345},\"id\":1}");
    ESP32HTTPClient client("http://example.com");
    auto req = client.jsonRpc("/rpc");
    req.method("test").id(1).execute();
    expectEq(req.getErrorData().c_str(), "12345", "Primitive error data number");
  }

  // Response ID parsed correctly when different from request
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":\"ok\",\"id\":99}");
    ESP32HTTPClient client("http://example.com");
    auto req = client.jsonRpc("/rpc");
    req.method("test").id(1).execute();
    expectEq(req.getResponseId().c_str(), "99", "Response ID 99 recorded");
  }

  // nullId(), clearId(), and rawParam with String
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":true,\"id\":null}");
    ESP32HTTPClient client("http://example.com");
    String rawStrVal = "{\"rawVal\":1}";
    client.jsonRpc("/rpc")
          .method("nullIdTest")
          .nullId()
          .rawParam(rawStrVal)
          .rawParam("namedRaw", rawStrVal)
          .execute();
    expectContains(HttpClientStub::lastPayload, "\"id\":null", "Payload has null id");
    expectContains(HttpClientStub::lastPayload, "{\"rawVal\":1}", "rawParam String payload");
  }

  // Path and query parameters with all numeric types
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":true,\"id\":1}");
    ESP32HTTPClient client("http://example.com");
    int iVal = 1;
    long lVal = 3;
    unsigned long ulVal = 4;
    float fVal = 7.5f;
    double dVal = 8.5;

    client.jsonRpc("/rpc/{i}/{l}/{ul}/{f}/{d}")
          .method("paramTest")
          .path("i", iVal)
          .path("l", lVal)
          .path("ul", ulVal)
          .path("f", fVal)
          .path("d", dVal)
          .queryParam("qi", iVal)
          .queryParam("ql", lVal)
          .queryParam("qul", ulVal)
          .queryParam("qf", fVal)
          .queryParam("qd", dVal)
          .execute();

    expectContains(HttpClientStub::lastUrl, "/rpc/1/3/4/", "Numeric path params replaced");
    expectContains(HttpClientStub::lastUrl, "qi=1", "Query param qi");
    expectContains(HttpClientStub::lastUrl, "qul=4", "Query param qul");
  }

  // Error targets and complex error data (object and array)
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32001,\"message\":\"Failed\",\"data\":{\"detail\":\"bad_data\"}},\"id\":1}");
    ESP32HTTPClient client("http://example.com");
    JsonRpcError errTarget;
    int errCodeTarget = 0;
    String errMsgTarget = "";
    String errDataTarget = "";

    client.jsonRpc("/rpc")
          .method("complexErr")
          .id(1)
          .getError(&errTarget)
          .getErrorCode(&errCodeTarget)
          .getErrorMessage(&errMsgTarget)
          .getErrorData(&errDataTarget)
          .execute();

    expectEqInt(errCodeTarget, -32001, "getErrorCode target");
    expectEq(errMsgTarget.c_str(), "Failed", "getErrorMessage target");
    expectContains(errDataTarget.c_str(), "bad_data", "getErrorData target contains bad_data");
    expectEqInt(errTarget.code, -32001, "getError target struct code");
  }

  // Raw result with string and numeric, combined with bindings
  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":\"hello raw\",\"id\":1}");
    ESP32HTTPClient client("http://example.com");
    String rawRes = "";
    String boundRes = "";
    client.jsonRpc("/rpc")
          .method("rawResultTest")
          .id(1)
          .getRawResult(&rawRes)
          .getResult(&boundRes)
          .execute();
    expectEq(rawRes.c_str(), "\"hello raw\"", "Raw result string");
    expectEq(boundRes.c_str(), "hello raw", "Bound result string");
  }

  {
    HttpClientStub::reset();
    HttpClientStub::setResponse(200, "{\"jsonrpc\":\"2.0\",\"result\":8888,\"id\":1}");
    ESP32HTTPClient client("http://example.com");
    String rawRes = "";
    int boundNum = 0;
    client.jsonRpc("/rpc")
          .method("rawResultNumTest")
          .id(1)
          .getRawResult(&rawRes)
          .getResult(&boundNum)
          .execute();
    expectEq(rawRes.c_str(), "8888", "Raw result number");
    expectEqInt(boundNum, 8888, "Bound result number");
  }
}

} // namespace

int main() {
  std::cout << "=== Running JSON-RPC Unit Tests ===\n\n";

  runSuite("JsonRpcBasicRequestFormatting", testBasicRequestFormatting);
  runSuite("JsonRpcPositionalParameters", testPositionalParameters);
  runSuite("JsonRpcNamedParameters", testNamedParameters);
  runSuite("JsonRpcConfigurableIds", testConfigurableIds);
  runSuite("JsonRpcNotifications", testNotifications);
  runSuite("JsonRpcPrimitiveResponseParsing", testPrimitiveResponseParsing);
  runSuite("JsonRpcObjectResponseParsing", testObjectResponseParsing);
  runSuite("JsonRpcArrayResponseParsing", testArrayResponseParsing);
  runSuite("JsonRpcStructSerializationAndDeserialization", testStructSerializationAndDeserialization);
  runSuite("JsonRpcRawResultAndRawResponse", testRawResultAndRawResponse);
  runSuite("JsonRpcErrorHandling", testErrorHandling);
  runSuite("JsonRpcStandardErrorCodes", testStandardErrorCodes);
  runSuite("JsonRpcHeadersAndAuth", testHeadersAndAuth);
  runSuite("JsonRpcUrlPathAndQueryParams", testUrlPathAndQueryParams);
  runSuite("JsonRpcRetryAndObservability", testRetryAndObservability);
  runSuite("JsonRpcStreamingLargeResponse", testStreamingLargeResponse);
  runSuite("JsonRpcMoveConstructor", testMoveConstructor);
  runSuite("JsonRpcClientMethods", testClientMethods);
  runSuite("JsonRpcParamOverloadsAndBuilderSetters", testParamOverloadsAndBuilderSetters);
  runSuite("JsonRpcResponseBindingsAndHeadersAllTypes", testResponseBindingsAndHeadersAllTypes);
  runSuite("JsonRpcCallbacksAndErrorDetails", testCallbacksAndErrorDetails);
  runSuite("JsonRpcEdgeCasesAndParsingAnomalies", testEdgeCasesAndParsingAnomalies);

  std::cout << "\n=== JSON-RPC Test Summary ===\n";
  std::cout << "Suites: " << suitesRun << " total | " << suitesPassed << " passed | " << (suitesRun - suitesPassed) << " failed\n";
  std::cout << "Checks: " << checks << " total | " << passedChecks << " passed | " << (checks - passedChecks) << " failed\n\n";

  return (failures == 0) ? 0 : 1;
}
