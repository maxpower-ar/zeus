#include "envio.h"
#include "config.h"
#include "secretos.h"  // si da error aca: falta copiar secretos_ejemplo.h como secretos.h
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <LineaServidor.h>

static const int HTTP_OK = 200;
static const int HTTP_REDIRECCION = 302;

// Hace un pedido HTTPS. Con cuerpo es POST; con cuerpo == nullptr es GET.
// Devuelve el codigo HTTP (negativo si fallo la conexion).
static int pedirHttps(const String& url, const char* cuerpo, String& respuesta,
                      String& redireccion) {
  WiFiClientSecure cliente;
  // TODO fase 10: validar el certificado del servidor. setInsecure() cifra la conexion pero
  // no comprueba que del otro lado este Google. Aceptable en banco, no en la version final.
  cliente.setInsecure();

  HTTPClient http;
  http.setTimeout(TIEMPO_ESPERA_HTTP_MS);
  // La redireccion se sigue a mano (ver enviarLote): asi se ve cada paso y no hay riesgo
  // de que el POST se repita.
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

  if (!http.begin(cliente, url)) {
    return -1;
  }

  int codigo;
  if (cuerpo != nullptr) {
    http.addHeader("Content-Type", "text/plain");
    codigo = http.POST((uint8_t*)cuerpo, strlen(cuerpo));
  } else {
    codigo = http.GET();
  }

  if (codigo == HTTP_OK) {
    respuesta = http.getString();
  } else if (codigo == HTTP_REDIRECCION) {
    redireccion = http.getLocation();
  } else if (codigo < 0) {
    respuesta = http.errorToString(codigo);
  }
  http.end();
  return codigo;
}

bool enviarLote(const Medicion* mediciones, int cantidad) {
  if (cantidad <= 0) return true;

  // Todas las lineas en un solo cuerpo, una por renglon.
  String cuerpo;
  cuerpo.reserve(cantidad * TAMANO_LINEA);
  char linea[TAMANO_LINEA];
  for (int i = 0; i < cantidad; i++) {
    if (!armarLinea(linea, sizeof(linea), CODIGO_ESTACION, mediciones[i])) {
      Serial.println("  ERROR: una linea no entro en el buffer");
      return false;
    }
    Serial.print("  ");
    Serial.println(linea);
    cuerpo += linea;
    cuerpo += '\n';
  }

  // Google Apps Script responde en dos pasos: el POST guarda y contesta 302 con otra
  // direccion; la respuesta ("OK ..." o "ERROR ...") se lee haciendo GET a esa direccion.
  String url = String(URL_SCRIPT) + "?clave=" + CLAVE_API;
  String respuesta;
  String redireccion;
  int codigo = pedirHttps(url, cuerpo.c_str(), respuesta, redireccion);
  Serial.printf("  paso 1 (POST): HTTP %d\n", codigo);

  if (codigo == HTTP_REDIRECCION && redireccion.length() > 0) {
    String sinUso;
    codigo = pedirHttps(redireccion, nullptr, respuesta, sinUso);
    Serial.printf("  paso 2 (GET):  HTTP %d\n", codigo);
  }

  respuesta.trim();
  Serial.print("  respuesta del servidor: ");
  Serial.println(respuesta.length() > 0 ? respuesta : String("(vacia)"));

  if (codigo != HTTP_OK || !respuesta.startsWith("OK")) {
    return false;
  }

  // "OK guardadas=G duplicadas=D": G + D tiene que ser exactamente lo que se mando. Si no,
  // algo no cuadra y es mas seguro reintentar (las repetidas el servidor las descarta).
  int guardadas = -1;
  int duplicadas = -1;
  sscanf(respuesta.c_str(), "OK guardadas=%d duplicadas=%d", &guardadas, &duplicadas);
  if (guardadas + duplicadas != cantidad) {
    Serial.printf("  AVISO: se mandaron %d y el servidor confirmo %d; se reintenta\n",
                  cantidad, guardadas + duplicadas);
    return false;
  }
  return true;
}
