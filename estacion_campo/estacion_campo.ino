/*
  Estacion meteorologica - NODO DE CAMPO
  Fase 1: sensores de temperatura y humedad en el banco.

  Que hace: cada 2.5 segundos lee el DHT22 y los dos LM35Z, e imprime las tres
  temperaturas juntas por el monitor serie para poder compararlas entre si y
  contra el multimetro UT890C.

  Al arrancar imprime tambien la MAC de la placa (pendiente de la fase 0).

  Conexionado: ver docs/hardware/pinout.md
  Monitor serie a 115200.
*/

#include <WiFi.h>
#include "config.h"
#include "sensores.h"

// Estado del parpadeo. Sin delay(): el bucle principal tiene que quedar libre.
unsigned long ultimoCambioLed = 0;
bool ledEncendido = false;

unsigned long ultimoMuestreo = 0;

// Imprime un valor, o "invalido" si el sensor no respondio.
// Un dato invalido nunca se disfraza de cero.
void imprimirLectura(const char *etiqueta, Lectura lectura, const char *unidad) {
  Serial.print(etiqueta);
  Serial.print("=");
  if (lectura.valido) {
    Serial.print(lectura.valor, 2);
    Serial.print(unidad);
  } else {
    Serial.print("invalido");
  }
  Serial.print("  ");
}

void setup() {
  Serial.begin(VELOCIDAD_SERIE);
  delay(500);  // Le da tiempo al puerto USB a levantarse antes del primer print.

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // La MAC que usa ESP-NOW es la de la interfaz STA.
  WiFi.mode(WIFI_STA);

  iniciarSensores();

  Serial.println();
  Serial.println("=== NODO DE CAMPO - fase 1 ===");
  Serial.print("MAC (STA): ");
  Serial.println(WiFi.macAddress());
  Serial.println("Anotar esta MAC en docs/bitacora.md");
  Serial.println("Comparar las tres temperaturas entre si y contra el UT890C.");
  Serial.println();
}

void loop() {
  unsigned long ahora = millis();

  unsigned long duracion = ledEncendido ? LED_ENCENDIDO_MS : LED_APAGADO_MS;
  if (ahora - ultimoCambioLed >= duracion) {
    ultimoCambioLed = ahora;
    ledEncendido = !ledEncendido;
    digitalWrite(PIN_LED, ledEncendido ? HIGH : LOW);
  }

  if (ahora - ultimoMuestreo >= INTERVALO_MUESTREO_MS) {
    ultimoMuestreo = ahora;

    LecturaDht dht = leerDht();
    Lectura lm35_1 = leerLm35(PIN_LM35_1, LM35_1_OFFSET_C);
    Lectura lm35_2 = leerLm35(PIN_LM35_2, LM35_2_OFFSET_C);

    imprimirLectura("dht_t", dht.temperatura, "C");
    imprimirLectura("dht_h", dht.humedad, "%");
    imprimirLectura("lm35_1", lm35_1, "C");
    imprimirLectura("lm35_2", lm35_2, "C");

    // Diferencia contra el DHT22, que es el sensor de referencia de los tres.
    if (dht.temperatura.valido && lm35_1.valido) {
      Serial.print("d1=");
      Serial.print(lm35_1.valor - dht.temperatura.valor, 2);
      Serial.print("  ");
    }
    if (dht.temperatura.valido && lm35_2.valido) {
      Serial.print("d2=");
      Serial.print(lm35_2.valor - dht.temperatura.valor, 2);
    }

    Serial.println();
  }
}
