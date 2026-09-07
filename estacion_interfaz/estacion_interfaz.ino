/*
  Estacion meteorologica - NODO INTERFAZ
  Fase 0: verificacion del entorno.

  Igual que el nodo de campo, pero con el parpadeo invertido para poder
  distinguir las dos placas a simple vista.

  Monitor serie a 115200.
*/

#include <WiFi.h>
#include "config.h"

unsigned long ultimoCambioLed = 0;
bool ledEncendido = false;

unsigned long ultimoAvisoMac = 0;

void mostrarMac() {
  Serial.print("[interfaz] MAC (STA): ");
  Serial.println(WiFi.macAddress());
}

void setup() {
  Serial.begin(VELOCIDAD_SERIE);
  delay(500);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // ESP-NOW usa la MAC de la interfaz STA.
  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.println("=== NODO INTERFAZ - fase 0 ===");
  mostrarMac();
  Serial.println("Anotar esta MAC en docs/bitacora.md");
}

void loop() {
  unsigned long ahora = millis();

  unsigned long duracion = ledEncendido ? LED_ENCENDIDO_MS : LED_APAGADO_MS;
  if (ahora - ultimoCambioLed >= duracion) {
    ultimoCambioLed = ahora;
    ledEncendido = !ledEncendido;
    digitalWrite(PIN_LED, ledEncendido ? HIGH : LOW);
  }

  if (ahora - ultimoAvisoMac >= INTERVALO_MAC_MS) {
    ultimoAvisoMac = ahora;
    mostrarMac();
  }
}
