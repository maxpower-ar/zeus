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

  if (MQ7_INSTALADO) {
    // 11 dB: el pin mide de 0 a ~3.1 V. analogReadMilliVolts usa la calibracion de fabrica
    // del ADC grabada en el chip, mas exacta que convertir analogRead a mano.
    analogSetPinAttenuation(PIN_MQ7, ADC_11db);
    Serial.println("[sensores] MQ-7: los primeros minutos despues de encender, y las primeras"
                   " 48 h de uso, da valores altos (el calefactor se esta estabilizando)");
  } else {
    Serial.println("[sensores] MQ-7: desactivado en config.h (CO = NaN)");
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

// MQ-7: el CO baja la resistencia Rs del sensor. Rs y la resistencia de carga RL del modulo
// forman un divisor: con mas CO, mas tension en AO. De la tension se despeja Rs, y de Rs/R0
// salen los ppm con la curva del datasheet.
static void leerMq7(Medicion& medicion, bool avisar) {
  if (!MQ7_INSTALADO) return;  // queda "sin sensor"

  uint32_t suma = 0;
  for (int i = 0; i < MQ7_MUESTRAS; i++) suma += analogReadMilliVolts(PIN_MQ7);
  int milivoltsPin = (int)(suma / MQ7_MUESTRAS);

  if (milivoltsPin < MQ7_MINIMO_MV) {
    anotarFalla(medicion.gas, medicion.estadoGas);
    if (avisar) {
      Serial.printf("[sensores] FALLA MQ-7: %d mV en GPIO39 -> ERR. Revisar el cable de AO, el"
                    " divisor y los 5 V del modulo\n", milivoltsPin);
    }
    return;
  }
  if (milivoltsPin > MQ7_MAXIMO_MV) {
    anotarFalla(medicion.gas, medicion.estadoGas);
    if (avisar) {
      Serial.printf("[sensores] FALLA MQ-7: %d mV en GPIO39, el ADC satura -> ERR. Revisar el"
                    " divisor (AO NUNCA directo al ESP32)\n", milivoltsPin);
    }
    return;
  }

  // Tension real en AO, antes del divisor.
  const float factorDivisor =
      (MQ7_DIVISOR_ARRIBA_KOHM + MQ7_DIVISOR_ABAJO_KOHM) / MQ7_DIVISOR_ABAJO_KOHM;
  float voltsAo = milivoltsPin / 1000.0f * factorDivisor;

  // El divisor queda en paralelo con RL: la carga real es la combinacion de las dos.
  const float divisorTotalKohm = MQ7_DIVISOR_ARRIBA_KOHM + MQ7_DIVISOR_ABAJO_KOHM;
  const float cargaKohm = MQ7_RESISTENCIA_CARGA_KOHM * divisorTotalKohm /
                          (MQ7_RESISTENCIA_CARGA_KOHM + divisorTotalKohm);

  if (voltsAo >= MQ7_TENSION_VCC_V) {
    anotarFalla(medicion.gas, medicion.estadoGas);
    if (avisar) {
      Serial.printf("[sensores] FALLA MQ-7: AO = %.2f V, igual o mas que VCC -> ERR. Revisar"
                    " MQ7_TENSION_VCC_V y el divisor en config.h\n", voltsAo);
    }
    return;
  }
  float rsKohm = cargaKohm * (MQ7_TENSION_VCC_V - voltsAo) / voltsAo;
  float relacion = rsKohm / MQ7_R0_KOHM;
  float ppm = MQ7_CURVA_A * powf(relacion, MQ7_CURVA_B);

  if (avisar) {
    Serial.printf("[sensores] MQ-7: %d mV en el pin -> AO %.2f V, Rs %.1f kOhm, Rs/R0 %.2f ->"
                  " %.0f ppm de CO (SIN CALIBRAR). R0 para aire limpio: %.2f kOhm\n",
                  milivoltsPin, voltsAo, rsKohm, relacion, ppm,
                  rsKohm / MQ7_AIRE_LIMPIO_RS_R0);
  }
  if (!(ppm >= 0.0f && ppm <= MQ7_PPM_MAXIMO)) {
    anotarFalla(medicion.gas, medicion.estadoGas);
    if (avisar) Serial.println("[sensores] FALLA MQ-7: ppm fuera de lo posible -> ERR");
    return;
  }
  anotarValor(medicion.gas, medicion.estadoGas, ppm);
}

void leerSensores(Medicion& medicion, bool avisar) {
  leerDht22(medicion, avisar);
  leerBmp180(medicion, avisar);
  leerMq7(medicion, avisar);
  // Direccion y luminosidad: sin sensor, quedan como vienen de medicionVacia (NaN).
}

float temperaturaBmp180() { return ultimaTemperaturaBmp; }
