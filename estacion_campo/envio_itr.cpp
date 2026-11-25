#include "envio_itr.h"
#include "config.h"
#include "secretos.h"  // si da error aca: falta copiar secretos.h (ver LEEME.md)
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <JsonItr.h>

static const int HTTP_OK = 200;
static const int HTTP_CREADO = 201;
static const int HTTP_PEDIDO_INVALIDO = 400;
static const int HTTP_NO_PROCESABLE = 422;

// La que trae secretos_ejemplo.h. Con esa su API contesta 401: no tiene sentido intentar.
static const char* const CLAVE_DE_EJEMPLO = "pegar-aca-la-clave-del-itr";

ResultadoItr enviarMedicionItr(const Medicion& medicion) {
  if (strcmp(CLAVE_API_ITR, CLAVE_DE_EJEMPLO) == 0) {
    Serial.println("  FALTA la clave del ITR: pegarla en CLAVE_API_ITR (secretos.h). Las"
                   " mediciones quedan guardadas para el ITR hasta que este");
    return ITR_FALLO;
  }

  char json[TAMANO_JSON_ITR];
  if (!armarJsonItr(json, sizeof(json), CODIGO_ESTACION, medicion)) {
    Serial.println("  ERROR: el JSON no entro en el buffer");
    return ITR_FALLO;
  }
  Serial.print("  ");
  Serial.println(json);

  WiFiClientSecure cliente;
  // TODO fase 10: validar el certificado del servidor (ver docs/servidor.md).
  cliente.setInsecure();

  HTTPClient http;
  http.setTimeout(TIEMPO_ESPERA_HTTP_MS);
  if (!http.begin(cliente, URL_ITR)) {
    Serial.println("  ERROR: no se pudo armar el pedido");
    return ITR_FALLO;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", CLAVE_API_ITR);

  int codigo = http.POST((uint8_t*)json, strlen(json));
  String respuesta = (codigo > 0) ? http.getString() : http.errorToString(codigo);
  http.end();

  respuesta.trim();
  Serial.printf("  HTTP %d  respuesta: %s\n", codigo,
                respuesta.length() > 0 ? respuesta.c_str() : "(vacia)");

  if (codigo == HTTP_OK || codigo == HTTP_CREADO) return ITR_ACEPTADA;
  if (codigo == HTTP_PEDIDO_INVALIDO || codigo == HTTP_NO_PROCESABLE) return ITR_RECHAZADA;
  return ITR_FALLO;
}
