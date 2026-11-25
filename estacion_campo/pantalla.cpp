#include "pantalla.h"
#include "config.h"
#include "red.h"
#include "registro.h"
#include "envio.h"
#include <Wire.h>
#include <Adafruit_GFX.h>      // libreria "Adafruit GFX Library"
#include <Adafruit_SSD1306.h>  // libreria "Adafruit SSD1306"
#include <LineaServidor.h>
#include <WiFi.h>

static const int ANCHO_PANTALLA = 128;
static const int ALTO_PANTALLA = 64;
static const int8_t SIN_PIN_RESET = -1;  // el modulo de 4 pines no tiene pin de reset
static const int ALTO_RENGLON = 8;       // letra de tamano 1: 6x8 pixeles, 21 letras por renglon
// Entran 21 letras por renglon, pero el buffer es mas grande: el compilador calcula el peor
// caso de cada numero (por ejemplo un anio de 11 cifras) y avisaria de un posible recorte.
static const size_t TAMANO_RENGLON = 80;

// Las dos frecuencias en FRECUENCIA_I2C_HZ: si no, la libreria deja el bus en 100 kHz
// despues de cada dibujo, y cambia la velocidad para el BMP180 y el DS3231.
static Adafruit_SSD1306 oled(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, SIN_PIN_RESET,
                             FRECUENCIA_I2C_HZ, FRECUENCIA_I2C_HZ);
static bool lista = false;
static bool hayMedicion = false;
static Medicion ultima;
static unsigned long ultimoDibujo = 0;

void iniciarPantalla() {
  if (!PANTALLA_INSTALADA) return;

  Wire.beginTransmission(DIRECCION_PANTALLA);
  if (Wire.endTransmission() != 0) {
    Serial.println("[pantalla] FALLA: no responde en 0x3C (revisar SDA, SCL y 3V3). La"
                   " estacion sigue sin pantalla");
    return;
  }
  // false al final: el bus I2C ya lo inicio el .ino con los pines y la velocidad de config.h.
  if (!oled.begin(SSD1306_SWITCHCAPVCC, DIRECCION_PANTALLA, true, false)) {
    Serial.println("[pantalla] FALLA: sin memoria para la pantalla");
    return;
  }
  lista = true;
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.println("Estacion meteorologica");
  oled.println(CODIGO_ESTACION);
  oled.println();
  oled.println("Arrancando...");
  oled.display();
  Serial.println("[pantalla] lista");
}

void pantallaMedicion(const Medicion& medicion) {
  ultima = medicion;
  hayMedicion = true;
}

// En la pantalla, "sin sensor" se ve como "--" (mas facil de leer que NaN); la falla como ERR.
static void datoEnPantalla(char* destino, size_t tamano, float valor, EstadoDato estado,
                           int decimales) {
  if (estado == DATO_SIN_SENSOR) {
    snprintf(destino, tamano, "--");
  } else {
    formatearDato(destino, tamano, valor, estado, decimales);
  }
}

static void renglon(int numero, const char* texto) {
  oled.setCursor(0, numero * ALTO_RENGLON);
  oled.print(texto);
}

static void dibujar() {
  char texto[TAMANO_RENGLON];
  oled.clearDisplay();

  time_t ahora;
  bool hayHora = obtenerHora(ahora);
  tm f;
  localtime_r(&ahora, &f);
  if (hayHora) {
    snprintf(texto, sizeof(texto), "%02d/%02d/%04d  %02d:%02d:%02d", f.tm_mday, f.tm_mon + 1,
             f.tm_year + 1900, f.tm_hour, f.tm_min, f.tm_sec);
  } else {
    snprintf(texto, sizeof(texto), "Sin hora (falta red)");
  }
  renglon(0, texto);

  if (wifiConectado()) {
    snprintf(texto, sizeof(texto), "WiFi OK  %d dBm", WiFi.RSSI());
  } else {
    snprintf(texto, sizeof(texto), "WiFi %s", textoEstadoWifi());
  }
  renglon(1, texto);

  if (hayMedicion) {
    time_t momento = (time_t)ultima.marcaTiempo;
    tm m;
    localtime_r(&momento, &m);
    snprintf(texto, sizeof(texto), "Medicion %02d:%02d", m.tm_hour, m.tm_min);
    renglon(2, texto);

    char a[12], b[12];
    datoEnPantalla(a, sizeof(a), ultima.temperatura, ultima.estadoTemperatura,
                   DECIMALES_TEMPERATURA);
    datoEnPantalla(b, sizeof(b), ultima.humedad, ultima.estadoHumedad, DECIMALES_HUMEDAD);
    snprintf(texto, sizeof(texto), "T %s C   H %s %%", a, b);
    renglon(3, texto);

    datoEnPantalla(a, sizeof(a), ultima.velocidad, ultima.estadoVelocidad, DECIMALES_VELOCIDAD);
    snprintf(texto, sizeof(texto), "Viento %s km/h", a);
    renglon(4, texto);

    datoEnPantalla(a, sizeof(a), ultima.presion, ultima.estadoPresion, DECIMALES_PRESION);
    datoEnPantalla(b, sizeof(b), ultima.gas, ultima.estadoGas, DECIMALES_GAS);
    snprintf(texto, sizeof(texto), "P %s hPa CO %s", a, b);  // "P 1013.2 hPa CO 1234"
    renglon(5, texto);
  } else {
    renglon(2, "Medicion: esperando");
  }

  if (destinoActivo(DESTINO_ITR)) {
    snprintf(texto, sizeof(texto), "Pend: %lu  ITR: %lu",
             (unsigned long)cantidadPendientes(DESTINO_SHEETS),
             (unsigned long)cantidadPendientes(DESTINO_ITR));
  } else {
    snprintf(texto, sizeof(texto), "Pendientes: %lu",
             (unsigned long)cantidadPendientes(DESTINO_SHEETS));
  }
  renglon(6, texto);

  time_t ok = ultimoEnvioOk(DESTINO_SHEETS);
  if (ultimoEnvioFallo(DESTINO_SHEETS)) {
    snprintf(texto, sizeof(texto), "Envio: FALLO");
  } else if (ok != 0) {
    tm e;
    localtime_r(&ok, &e);
    snprintf(texto, sizeof(texto), "Envio OK %02d:%02d", e.tm_hour, e.tm_min);
  } else {
    snprintf(texto, sizeof(texto), "Envio: todavia no");
  }
  renglon(7, texto);

  oled.display();
}

void atenderPantalla() {
  if (!lista) return;
  if (millis() - ultimoDibujo < INTERVALO_PANTALLA_MS) return;
  ultimoDibujo = millis();
  dibujar();
}
