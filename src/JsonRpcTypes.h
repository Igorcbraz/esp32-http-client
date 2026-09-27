#ifndef JSON_RPC_TYPES_H
#define JSON_RPC_TYPES_H

#include <Arduino.h>
#include <functional>
#include <vector>

/**
 * Standard JSON-RPC 2.0 Pre-defined Error Codes
 */
enum JsonRpcErrorCode {
  JSONRPC_PARSE_ERROR        = -32700,  // Invalid JSON was received by the server.
  JSONRPC_INVALID_REQUEST    = -32600,  // The JSON sent is not a valid Request object.
  JSONRPC_METHOD_NOT_FOUND   = -32601,  // The method does not exist / is not available.
  JSONRPC_INVALID_PARAMS     = -32602,  // Invalid method parameter(s).
  JSONRPC_INTERNAL_ERROR     = -32603,  // Internal JSON-RPC error.
  JSONRPC_SERVER_ERROR_START = -32099,  // Reserved for implementation-defined server-errors.
  JSONRPC_SERVER_ERROR_END   = -32000
};

/**
 * Structure representing a JSON-RPC 2.0 Error object
 */
struct JsonRpcError {
  int code = 0;
  String message = "";
  String data = "";

  bool hasError() const {
    return code != 0 || !message.isEmpty();
  }

  void clear() {
    code = 0;
    message = "";
    data = "";
  }
};

typedef std::function<void(const JsonRpcError&)> JsonRpcErrorCallback;

enum JsonRpcParamMode {
  JSONRPC_PARAMS_NONE,
  JSONRPC_PARAMS_POSITIONAL,
  JSONRPC_PARAMS_NAMED,
  JSONRPC_PARAMS_RAW
};

enum JsonRpcIdType {
  JSONRPC_ID_AUTO,
  JSONRPC_ID_INT,
  JSONRPC_ID_STRING,
  JSONRPC_ID_NULL
};

#endif
