---
title: Referência da Classe JsonRpcRequest - Cliente JSON-RPC 2.0 Fluente para ESP32
description: Documentação da classe JsonRpcRequest: encadeamento de métodos para requisições JSON-RPC 2.0, parâmetros posicionais e nomeados, IDs, notificações e tratamento de erros.
keywords: JsonRpcRequest ESP32, cliente JSON-RPC ESP32, JSON-RPC HTTP, notificações JSON-RPC, erros JSON-RPC
tags:
  - api
  - jsonrpc
  - request
  - class
---
# JsonRpcRequest

O construtor fluente de requisições retornado por `.jsonRpc(path)` e `.jsonrpc(path)` no [`ESP32HTTPClient`](esp32httpclient.pt.md). Todos os métodos retornam `JsonRpcRequest&`, permitindo chamadas encadeadas.

**A requisição JSON-RPC é enviada automaticamente** quando o objeto `JsonRpcRequest` sai de escopo (ao final da instrução) ou quando `.execute()` é chamado explicitamente.

!!! note "Semântica de cópia"
    `JsonRpcRequest` é **apenas movimentável (move-only)** — não pode ser copiado, garantindo baixo uso de memória e segurança.

---

## Visão Geral de Encadeamento

```cpp
ESP32HTTPClient client("https://api.example.com");

int soma = 0;
client.jsonRpc("/rpc")
    .method("add")
    .param(15)
    .param(27)
    .id(1)
    .getResult(&soma);
```

---

## Configuração do Método e ID

### `method(name)`
Define o nome do método remoto a ser executado.
```cpp
JsonRpcRequest& method(const char* methodName);
JsonRpcRequest& method(const String& methodName);
```

### `id(reqId)`
Define um ID configurável para a requisição (inteiro ou string).
```cpp
JsonRpcRequest& id(int reqId);
JsonRpcRequest& id(long reqId);
JsonRpcRequest& id(const char* reqId);
JsonRpcRequest& id(const String& reqId);
```

### `nullId()`
Define explicitamente o campo `"id"` como `null`.
```cpp
JsonRpcRequest& nullId();
```

### `notification()` / `asNotification()`
Configura a requisição como uma Notificação JSON-RPC 2.0. O campo `"id"` é omitido do payload e o servidor não retorna resposta.
```cpp
JsonRpcRequest& notification(bool enable = true);
JsonRpcRequest& asNotification();
```

---

## Parâmetros

Suporta parâmetros posicionais (vetor), nomeados (objeto), JSON bruto e structs C++ com `REST_JSON_MAP`.

### Parâmetros Posicionais (Array)
Ao chamar `.param(val)` ou `.positionalParam(val)`, os valores são serializados em um array JSON:
```cpp
client.jsonRpc("/rpc")
    .method("subtract")
    .param(42)
    .param(23);
// Payload JSON: {"jsonrpc":"2.0","method":"subtract","params":[42,23],"id":1}
```

### Parâmetros Nomeados (Objeto)
Ao chamar `.param(name, val)` ou `.namedParam(name, val)`, os valores são estruturados como pares chave-valor:
```cpp
client.jsonRpc("/rpc")
    .method("subtract")
    .param("minuend", 42)
    .param("subtrahend", 23);
// Payload JSON: {"jsonrpc":"2.0","method":"subtract","params":{"minuend":42,"subtrahend":23},"id":1}
```

---

## Leitura e Binding de Resultados

Os resultados podem ser mapeados diretamente para variáveis via streaming com zero alocações redundantes:

### Resultado Primitivo
```cpp
int resultado = 0;
client.jsonRpc("/rpc").method("add").param(10).param(5).getResult(&resultado);
```

### Subcampos de Objeto
```cpp
float temp = 0.0f;
String unit = "";
client.jsonRpc("/rpc")
    .method("getWeather")
    .getResult("temperature", &temp)
    .getResult("unit", &unit);
```

### Structs
```cpp
struct Weather {
  float temperature;
  String unit;
  REST_JSON_MAP(REST_FIELD(temperature), REST_FIELD(unit))
};

Weather w;
client.jsonRpc("/rpc").method("getWeather").getResult(&w);
```

---

## Tratamento de Erros

Respostas de erro padrão JSON-RPC 2.0 são identificadas e disponibilizadas:

```cpp
client.jsonRpc("/rpc")
    .method("invalidMethod")
    .onJsonRpcError([](const JsonRpcError& err) {
      Serial.printf("Erro JSON-RPC %d: %s\n", err.code, err.message.c_str());
    });
```
