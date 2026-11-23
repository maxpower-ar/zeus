#include "sensores.h"
#include "config.h"
#include <Wire.h>
#include <DHT.h>                 // libreria "DHT sensor library" (Adafruit)
#include <Adafruit_BMP085.h>     // libreria "Adafruit BMP085 Library": sirve para el BMP180

static DHT dht(PIN_DHT, DHT22);
static Adafruit_BMP085 bmp;
static bool bmpIniciado = false;
static float ultimaTemperaturaBmp = NAN;

// Pregunta en el bus I2C si hay alguien en esa direccion. Es lo que hace un escaner I2C.
static bool respondeEnI2c(uint8_t direccion) {
  Wire.beginTransmission(direccion);
  return Wire.endTransmission() == 0;
}

void iniciarSensores() {
  if (DHT22_INSTALADO) dht.begin();

  if (BMP180_INSTALADO) {
    // Si no arranca aca no se detiene nada: se reintenta en cada medicion (leerBmp180).
    bmpIniciado = respondeEnI2c(DIRECCION_BMP180) && bmp.begin(BMP085_ULTRAHIGHRES);
    Serial.printf("[sensores] BMP180: %s\n",
                  bmpIniciado ? "detectado" : "NO detectado en 0x77 (revisar cables SDA/SCL y 3V3)");
  } else {
    Serial.println("[sensores] BMP180: desactivado en config.h (presion = NaN)");
  }
}

static void leerDht22(Medicion& medicion, bool avisar) {
  if (!DHT22_INSTALADO) return;  // quedan "sin sensor"

  // La libreria devuelve NaN si el sensor no contesta o si el dato llego con el CRC mal.
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  // Un numero fuera del rango del sensor tampoco es una medicion: es una falla.
  bool temperaturaFueraDeRango = !isnan(temperatura) &&
      (temperatura < DHT22_TEMPERATURA_MINIMA || temperatura > DHT22_TEMPERATURA_MAXIMA);
  bool humedadFueraDeRango = !isnan(humedad) &&
      (humedad < DHT22_HUMEDAD_MINIMA || humedad > DHT22_HUMEDAD_MAXIMA);
  if (temperaturaFueraDeRango) temperatura = NAN;
  if (humedadFueraDeRango) humedad = NAN;

  anotarValor(medicion.temperatura, medicion.estadoTemperatura, temperatura);
  anotarValor(medicion.humedad, medicion.estadoHumedad, humedad);

  if (!avisar) return;
  if (medicion.estadoTemperatura == DATO_FALLA || medicion.estadoHumedad == DATO_FALLA) {
    if (temperaturaFueraDeRango || humedadFueraDeRango) {
      Serial.println("[sensores] FALLA DHT22: lectura fuera del rango del sensor -> ERR");
    } else {
      Serial.println("[sensores] FALLA DHT22: no responde o dato con CRC incorrecto -> ERR."
                     " Revisar el cable de GPIO4");
    }
  }
}

static void leerBmp180(Medicion& medicion, bool avisar) {
  ultimaTemperaturaBmp = NAN;
  if (!BMP180_INSTALADO) return;  // queda "sin sensor"

  // Antes de leer se pregunta si esta: la libreria no avisa si el sensor se desconecto,
  // devolveria un numero cualquiera.
  if (!respondeEnI2c(DIRECCION_BMP180)) {
    bmpIniciado = false;  // al volver, hay que leerle de nuevo la calibracion de fabrica
    anotarFalla(medicion.presion, medicion.estadoPresion);
    if (avisar) {
      Serial.println("[sensores] FALLA BMP180: no responde en 0x77 -> ERR. Revisar SDA (GPIO21),"
                     " SCL (GPIO22) y 3V3");
    }
    return;
  }
  if (!bmpIniciado) {
    bmpIniciado = bmp.begin(BMP085_ULTRAHIGHRES);
    if (!bmpIniciado) {
      anotarFalla(medicion.presion, medicion.estadoPresion);
      if (avisar) {
        Serial.println("[sensores] FALLA BMP180: algo responde en 0x77 pero no es un BMP180"
                       " -> ERR");
      }
      return;
    }
  }

  // Presion ABSOLUTA de la estacion, la que hay a la altura a la que esta. No se corrige a
  // nivel del mar: para eso hace falta la altitud medida, y todavia no la tenemos.
  float presionHpa = bmp.readPressure() * PA_A_HPA;
  ultimaTemperaturaBmp = bmp.readTemperature();

  if (presionHpa < BMP180_PRESION_MINIMA_HPA || presionHpa > BMP180_PRESION_MAXIMA_HPA) {
    anotarFalla(medicion.presion, medicion.estadoPresion);
    if (avisar) {
      Serial.printf("[sensores] FALLA BMP180: presion fuera de rango (%.1f hPa) -> ERR\n",
                    presionHpa);
    }
    return;
  }
  anotarValor(medicion.presion, medicion.estadoPresion, presionHpa);
  if (avisar) {
    Serial.printf("[sensores] BMP180: %.1f hPa (absoluta), %.1f C (solo para comparar con el"
                  " DHT22, no se envia)\n", presionHpa, ultimaTemperaturaBmp);
  }
}

void leerSensores(Medicion& medicion, bool avisar) {
  leerDht22(medicion, avisar);
  leerBmp180(medicion, avisar);
  // Gas, direccion y luminosidad: sin sensor, quedan como vienen de medicionVacia (NaN).
}

float temperaturaBmp180() { return ultimaTemperaturaBmp; }
