/*
  PRUEBA DE BANCO - DHT22 (temperatura y humedad) en GPIO4

  Sketch minimo para verificar el sensor. Requiere instalar dos librerias:
  Herramientas -> Administrar bibliotecas -> buscar "DHT sensor library" (Adafruit),
  y aceptar cuando ofrezca instalar las dependencias (Adafruit Unified Sensor).

  Conexionado:
    VCC  -> 3V3   (el DHT22 trabaja a 3.3 V, NO a 5 V)
    DATA -> GPIO4
    GND  -> GND

  Si el DHT22 es de 4 patas peladas hace falta ademas una resistencia de 10k entre
  DATA y 3V3. Si viene sobre una plaquita de 3 patas, esa resistencia ya esta adentro.

  El DHT22 no admite mas de una lectura cada 2 segundos.

  Monitor serie a 115200.
*/

#include <DHT.h>

const int PIN_DHT = 4;

DHT dht(PIN_DHT, DHT22);

void setup() {
  Serial.begin(115200);
  delay(500);
  dht.begin();

  Serial.println();
  Serial.println("=== Prueba DHT22 en GPIO4 ===");
  Serial.println("Sopla sobre el sensor: la humedad tiene que subir de golpe.");
  Serial.println();
}

void loop() {
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  // La libreria devuelve NaN cuando el sensor no contesta o el dato llego corrupto.
  // Un sensor que no responde tiene que decirlo, no devolver cero.
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("lectura INVALIDA - el sensor no respondio");
  } else {
    Serial.print("temperatura = ");
    Serial.print(temperatura, 1);
    Serial.print(" C   humedad = ");
    Serial.print(humedad, 1);
    Serial.println(" %");
  }

  delay(2500);
}
