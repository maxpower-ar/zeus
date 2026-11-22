/*
  PRUEBA DE CONCEPTO - Envio de mediciones a Google Sheets por WiFi

  Cada minuto lee el DHT22, arma la linea del formato acordado (docs/servidor.md) y la
  manda al script de Google Apps Script, que la guarda como fila en la planilla.

  NO es el firmware de la estacion: no guarda nada en memoria, asi que una medicion que no
  se pudo enviar se pierde. El guardado y reenvio de pendientes son las fases 6 y 9.

  Antes de compilar:
    1. Instalar el servidor en Google (servidor/LEEME.md)
    2. Copiar secretos_ejemplo.h como secretos.h en esta carpeta y completarlo
    3. Librerias: "DHT sensor library" (Adafruit). WiFi, HTTPClient y WiFiClientSecure
       vienen con el soporte de ESP32. EstacionComun viene en el repositorio.

  Conexionado: el mismo de pruebas/dht22 (DATA en GPIO4). Sin el DHT22 conectado tambien
  funciona: temperatura y humedad salen como NaN y quedan como celdas vacias.

  Monitor serie a 115200.
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include <DHT.h>
#include <LineaServidor.h>
#include "secretos.h"  // si da error aca: falta copiar secretos_ejemplo.h como secretos.h

const int PIN_DHT = 4;
const char* const CODIGO_ESTACION = "EM_ITR_001";

const unsigned long INTERVALO_ENVIO_MS = 60000;        // 1 min para probar; la estacion: 5 min
const unsigned long INTERVALO_AVISO_WIFI_MS = 10000;
const uint16_t TIEMPO_ESPERA_HTTP_MS = 15000;

// Argentina: UTC-3 todo el ano. En la notacion POSIX el signo va al reves.
const char* const ZONA_HORARIA = "<-03>3";
const char* const SERVIDOR_NTP_1 = "pool.ntp.org";
const char* const SERVIDOR_NTP_2 = "time.google.com";
const int ANIO_MINIMO_VALIDO = 2025;  // un reloj que marca antes de esto todavia no sincronizo

const int HTTP_OK = 200;
const int HTTP_REDIRECCION = 302;

DHT dht(PIN_DHT, DHT22);

unsigned long ultimoEnvio = 0;
unsigned long ultimoAvisoWifi = 0;
bool estabaConectado = false;
bool horaAvisada = false;
int enviosOk = 0;
int enviosFallidos = 0;


// Hace un pedido HTTPS. Con cuerpo es POST; con cuerpo == nullptr es GET.
// Devuelve el codigo HTTP (negativo si fallo la conexion).
int pedirHttps(const String& url, const char* cuerpo, String& respuesta, String& redireccion) {
  WiFiClientSecure cliente;
  // TODO fase 10: validar el certificado del servidor. setInsecure() cifra la conexion pero
  // no comprueba que del otro lado este Google. Aceptable en banco, no en la version final.
  cliente.setInsecure();

  HTTPClient http;
  http.setTimeout(TIEMPO_ESPERA_HTTP_MS);
  // La redireccion se sigue a mano (ver enviarLinea): asi se ve cada paso y no hay riesgo
  // de que el POST se repita y genere una fila duplicada.
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

// Manda una linea. Devuelve true solo si el servidor confirmo que la guardo.
//
// Google Apps Script responde en dos pasos: el POST guarda el dato y contesta 302 con otra
// direccion; la respuesta ("OK ..." o "ERROR ...") se lee haciendo GET a esa direccion.
bool enviarLinea(const char* linea) {
  String url = String(URL_SCRIPT) + "?clave=" + CLAVE_API;
  String respuesta;
  String redireccion;

  int codigo = pedirHttps(url, linea, respuesta, redireccion);
  Serial.printf("  paso 1 (POST): HTTP %d\n", codigo);

  if (codigo == HTTP_REDIRECCION && redireccion.length() > 0) {
    String sinUso;
    codigo = pedirHttps(redireccion, nullptr, respuesta, sinUso);
    Serial.printf("  paso 2 (GET):  HTTP %d\n", codigo);
  }

  respuesta.trim();
  Serial.print("  respuesta del servidor: ");
  Serial.println(respuesta.length() > 0 ? respuesta : String("(vacia)"));

  return codigo == HTTP_OK && respuesta.startsWith("OK");
}

// Muestra los cambios de estado del WiFi. La reconexion la hace el ESP32 solo
// (setAutoReconnect), sin trabar el loop.
void vigilarWifi() {
  bool conectado = WiFi.status() == WL_CONNECTED;
  if (conectado && !estabaConectado) {
    Serial.print("[wifi] conectado. IP: ");
    Serial.print(WiFi.localIP());
    Serial.printf("  senal: %d dBm\n", WiFi.RSSI());
  } else if (!conectado && estabaConectado) {
    Serial.println("[wifi] se perdio la conexion, reintentando...");
  }
  if (!conectado && millis() - ultimoAvisoWifi >= INTERVALO_AVISO_WIFI_MS) {
    ultimoAvisoWifi = millis();
    Serial.printf("[wifi] esperando conexion a \"%s\"...\n", WIFI_NOMBRE);
  }
  estabaConectado = conectado;
}

// Devuelve false mientras NTP no haya puesto el reloj en hora.
bool obtenerHora(tm& fechaHora) {
  if (!getLocalTime(&fechaHora, 10)) return false;
  return fechaHora.tm_year + 1900 >= ANIO_MINIMO_VALIDO;
}

void medirYEnviar() {
  tm fechaHora;
  if (!obtenerHora(fechaHora)) {
    Serial.println("[envio] todavia no hay hora de NTP, no se envia (hace falta WiFi)");
    return;
  }

  // Todo arranca en "sin dato"; solo se completa lo que hay. Viento, gas, direccion,
  // presion y luminosidad quedan en NaN (ver pruebas/registro_y_envio para el anemometro).
  Medicion medicion = medicionVacia((uint32_t)time(nullptr));
  medicion.temperatura = dht.readTemperature();  // NaN si el sensor no responde
  medicion.humedad = dht.readHumidity();

  char linea[TAMANO_LINEA];
  if (!armarLinea(linea, sizeof(linea), CODIGO_ESTACION, medicion)) {
    Serial.println("[envio] ERROR: la linea no entro en el buffer");
    return;
  }

  Serial.print("[envio] ");
  Serial.println(linea);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("  sin WiFi: esta medicion se pierde (en esta prueba no hay buffer)");
    enviosFallidos++;
    return;
  }

  if (enviarLinea(linea)) {
    enviosOk++;
    Serial.println("  -> GUARDADA en la planilla");
  } else {
    enviosFallidos++;
    Serial.println("  -> FALLO el envio");
  }
  Serial.printf("  total: %d ok, %d fallidos\n\n", enviosOk, enviosFallidos);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  dht.begin();

  Serial.println();
  Serial.println("=== Prueba de envio a Google Sheets ===");
  Serial.printf("Estacion %s, un envio cada %lu s\n\n", CODIGO_ESTACION, INTERVALO_ENVIO_MS / 1000);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_NOMBRE, WIFI_CLAVE);

  // NTP sincroniza solo, en segundo plano, apenas haya WiFi.
  configTzTime(ZONA_HORARIA, SERVIDOR_NTP_1, SERVIDOR_NTP_2);

  // Primer envio apenas haya hora, sin esperar un intervalo entero.
  ultimoEnvio = millis() - INTERVALO_ENVIO_MS;
}

void loop() {
  vigilarWifi();

  tm ahora;
  bool hayHora = obtenerHora(ahora);
  if (hayHora && !horaAvisada) {
    horaAvisada = true;
    Serial.printf("[ntp] hora sincronizada: %02d/%02d/%04d %02d:%02d\n\n",
                  ahora.tm_mday, ahora.tm_mon + 1, ahora.tm_year + 1900,
                  ahora.tm_hour, ahora.tm_min);
  }

  if (hayHora && millis() - ultimoEnvio >= INTERVALO_ENVIO_MS) {
    ultimoEnvio = millis();
    medirYEnviar();
  }
}
