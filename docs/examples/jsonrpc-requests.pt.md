---
title: Exemplo de Web Services JSON-RPC 2.0 - Cliente HTTP ESP32
description: Guia completo e exemplos demonstrando requisições JSON-RPC 2.0, parâmetros posicionais e nomeados, notificações e mapeamento de structs no ESP32.
keywords: Exemplo JSON-RPC ESP32, JSON-RPC 2.0 Arduino, cliente JSON-RPC ESP32, JsonRpcRequest
tags:
  - examples
  - jsonrpc
  - tutorial
---
# Consumindo Web Services JSON-RPC 2.0

O `ESP32-HTTP-Client` oferece suporte nativo e em streaming com zero heap para o protocolo **JSON-RPC 2.0** sobre HTTP e HTTPS. Ele serializa automaticamente os envelopes de requisição JSON-RPC 2.0, suporta parâmetros posicionais e nomeados, processa notificações e extrai os resultados diretamente para variáveis C++ e structs mapeadas.

---

## 1. Parâmetros Posicionais (Array)

No JSON-RPC 2.0, quando os parâmetros são ordenados como um array (`"params": [15, 27]`), utilize `.param(valor)` ou `.addParam(valor)`:

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://api.example.com");

void setup() {
  Serial.begin(115200);
  WiFi.begin("SEU_SSID", "SUA_SENHA");
  while (WiFi.status() != WL_CONNECTED) delay(500);

  int soma = 0;

  // Envia: {"jsonrpc":"2.0","method":"add","params":[15,27],"id":1}
  client.jsonRpc("/rpc")
        .method("add")
        .param(15)
        .param(27)
        .id(1)
        .getResult(&soma);

  if (client.isSuccess()) {
    Serial.printf("Resultado da soma: %d\n", soma);
  }
}

void loop() {}
```

---

## 2. Parâmetros Nomeados (Objeto)

Quando o servidor espera parâmetros por nome (`"params": {"minuend": 42, "subtrahend": 23}`), utilize `.param(chave, valor)` ou `.setParam(chave, valor)`:

```cpp
int resultadoSub = 0;

// Envia: {"jsonrpc":"2.0","method":"subtract","params":{"minuend":42,"subtrahend":23},"id":"sub-1"}
client.jsonRpc("/rpc")
      .method("subtract")
      .param("minuend", 42)
      .param("subtrahend", 23)
      .id("sub-1")
      .getResult(&resultadoSub);

Serial.printf("Resultado da subtração: %d\n", resultadoSub);
```

---

## 3. Notificações (Sem Resposta)

De acordo com a especificação JSON-RPC 2.0, uma **Notificação** é uma requisição sem a propriedade `id`. O cliente não aguarda nem processa resposta para notificações:

```cpp
// Envia: {"jsonrpc":"2.0","method":"logTelemetry","params":{"node":"esp32","uptime":120000}}
client.jsonRpc("/rpc")
      .method("logTelemetry")
      .asNotification()
      .param("node", "esp32")
      .param("uptime", 120000);
```

---

## 4. Mapeamento de Structs com `REST_JSON_MAP`

Você pode passar structs C++ diretamente nos parâmetros JSON-RPC ou vincular o objeto `result` diretamente a uma struct sem alocações dinâmicas de documentos JSON:

```cpp
struct LeituraSensor {
  char sensor[32] = {0};
  float temperatura = 0.0f;
  float umidade = 0.0f;

  REST_JSON_MAP(
    REST_FIELD(sensor),
    REST_FIELD(temperatura),
    REST_FIELD(umidade)
  )
};

// 1. Enviar struct como objeto de parâmetros:
LeituraSensor dados = {"DHT22", 24.5f, 55.0f};
client.jsonRpc("/rpc")
      .method("saveReading")
      .setParams(dados)
      .id(101);

// 2. Desserializar resultado diretamente para a struct:
LeituraSensor ultimaLeitura;
client.jsonRpc("/rpc")
      .method("getLatestReading")
      .param("sensorId", 1)
      .id(102)
      .getResult(&ultimaLeitura);

Serial.printf("Sensor: %s, Temp: %.1f C\n", ultimaLeitura.sensor, ultimaLeitura.temperatura);
```

---

## 5. Tratamento de Erros e Códigos Padrão

O JSON-RPC 2.0 retorna erros estruturados no objeto `error` (`{"code": -32601, "message": "Method not found"}`). A biblioteca disponibiliza constantes padrão e métodos de inspeção:

```cpp
JsonRpcError rpcError;
int saida = 0;

client.jsonRpc("/rpc")
      .method("metodoInexistente")
      .id(5)
      .getError(&rpcError)
      .onJsonRpcError([](const JsonRpcError& err) {
          Serial.printf("Erro JSON-RPC [%d]: %s\n", err.code, err.message.c_str());
      })
      .getResult(&saida);

if (rpcError.code == JSONRPC_ERR_METHOD_NOT_FOUND) {
  Serial.println("O método solicitado não existe no servidor.");
}
```

### Códigos de Erro Padrão JSON-RPC

| Constante | Código | Significado |
| :--- | :--- | :--- |
| `JSONRPC_ERR_PARSE_ERROR` | `-32700` | JSON inválido recebido pelo servidor. |
| `JSONRPC_ERR_INVALID_REQUEST` | `-32600` | O JSON enviado não é um objeto Request válido. |
| `JSONRPC_ERR_METHOD_NOT_FOUND` | `-32601` | O método não existe ou não está disponível. |
| `JSONRPC_ERR_INVALID_PARAMS` | `-32602` | Parâmetros de método inválidos. |
| `JSONRPC_ERR_INTERNAL_ERROR` | `-32603` | Erro interno do servidor JSON-RPC. |
| `JSONRPC_ERR_SERVER_ERROR_START` até `END` | `-32099` a `-32000` | Reservados para erros definidos pelo servidor. |

---

## 6. Extração de Campos Aninhados e JSON Bruto

Utilize a notação de ponto para vincular campos dentro do objeto `result`, ou capture a string bruta do JSON:

```cpp
char cidade[64] = {0};
String jsonResultadoBruto;
String corpoHttpCompleto;

client.jsonRpc("/rpc")
      .method("getUser")
      .param("id", 42)
      .id("u42")
      .getResult("address.city", cidade, sizeof(cidade)) // Extrai campo aninhado do result
      .getRawResult(&jsonResultadoBruto)                  // Extrai o objeto/array "result" bruto
      .getRawResponse(&corpoHttpCompleto);                // Extrai o corpo completo da resposta HTTP
```

---

## 7. Exemplo Completo (Sketch)

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_SSID";
const char* password = "SUA_SENHA";

ESP32HTTPClient client("https://api.example.com");

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");

  // Opcional: Autenticação Bearer ou cabeçalhos personalizados
  client.bearer("seu_token_api");

  // 1. Chamada com parâmetros posicionais
  int soma = 0;
  client.jsonRpc("/rpc")
        .method("add")
        .param(15)
        .param(27)
        .id(1)
        .getResult(&soma);

  Serial.printf("add(15, 27) = %d\n", soma);

  // 2. Chamada com parâmetros nomeados e tratamento de erros
  int diferenca = 0;
  client.jsonRpc("/rpc")
        .method("subtract")
        .param("minuend", 50)
        .param("subtrahend", 12)
        .id("sub-1")
        .getResult(&diferenca)
        .onJsonRpcError([](const JsonRpcError& err) {
            Serial.printf("Erro [%d]: %s\n", err.code, err.message.c_str());
        });

  Serial.printf("subtract(50, 12) = %d\n", diferenca);

  // 3. Notificação (sem id, fire-and-forget)
  client.jsonRpc("/rpc")
        .method("heartbeat")
        .asNotification()
        .param("node", "esp32");
}

void loop() {}
```
