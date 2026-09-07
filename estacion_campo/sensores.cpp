#include "sensores.h"
#include "config.h"

#include <DHT.h>

// Requiere las librerias "DHT sensor library" (Adafruit) y "Adafruit Unified Sensor".
static DHT dht(PIN_DHT, DHT22);

void iniciarSensores() {
  dht.begin();

  // Escala del ADC: 0 db equivale a un fondo de escala de ~1100 mV.
  // El LM35 nunca pasa de 1000 mV en el rango que nos interesa, asi que con esta
  // escala se aprovecha todo el conversor en la zona util.
  analogSetPinAttenuation(PIN_LM35_1, ADC_0db);
  analogSetPinAttenuation(PIN_LM35_2, ADC_0db);
}

LecturaDht leerDht() {
  LecturaDht resultado;

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // La libreria devuelve NaN cuando el sensor no contesta o el dato llego corrupto.
  resultado.temperatura.valido = !isnan(t);
  resultado.temperatura.valor = resultado.temperatura.valido ? t : 0.0;

  resultado.humedad.valido = !isnan(h);
  resultado.humedad.valor = resultado.humedad.valido ? h : 0.0;

  return resultado;
}

Lectura leerLm35(int pin, float offset) {
  Lectura resultado;

  // Promediado: una sola lectura del ADC salta varios grados de una vez a la otra.
  // analogReadMilliVolts aplica la curva de calibracion de fabrica del chip, que es
  // bastante mejor que convertir la cuenta cruda con una regla de tres.
  float suma = 0.0;
  for (int i = 0; i < LM35_MUESTRAS; i++) {
    suma += analogReadMilliVolts(pin);
    delayMicroseconds(200);
  }
  float milivoltios = suma / LM35_MUESTRAS;

  resultado.valido = (milivoltios >= LM35_MV_MINIMO && milivoltios <= LM35_MV_MAXIMO);
  resultado.valor = resultado.valido
                    ? (milivoltios / LM35_MV_POR_GRADO) + offset
                    : 0.0;

  return resultado;
}
