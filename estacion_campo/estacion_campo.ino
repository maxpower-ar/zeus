/*
  ESTACION METEOROLOGICA - firmware final (placa Delta)

  Cada INTERVALO_MEDICION_S (5 minutos, alineado al reloj: 10:00, 10:05, 10:10...) mide,
  GUARDA la medicion en la memoria flash y recien despues intenta mandarla. Medir y guardar
  nunca depende de la red: sin WiFi las mediciones quedan pendientes y se mandan solas al
  volver la conexion, primero las mas viejas.

  Cada dato sale como numero, NaN (sensor no instalado) o ERR (sensor instalado que fallo).

  Archivos (uno por responsabilidad):
    config.h         que hay instalado, pines, intervalos, calibraciones
    secretos.h       WiFi y claves. NO va al repositorio (copiar secretos_ejemplo.h)
    sensores         DHT22 (temperatura, humedad), BMP180 (presion) y MQ-7 (CO)
  Que cable va a que pin: hardware/conexiones.md
    anemometro       encoder optico, velocidad del viento
    registro         memoria flash: guardar primero, mandar despues
    red              WiFi y hora (NTP; DS3231 de respaldo en rtc)
    envio            cola de envio y reintentos -> envio_sheets (planilla), envio_itr (ITR)
    pantalla         OLED con la ultima medicion, WiFi, pendientes y hora
  El formato del dato y de la linea al servidor viven en libraries/EstacionComun.

  Antes de compilar: ver estacion_campo/LEEME.md (librerias, esquema de particiones,
  secretos.h).

  Teclas en el monitor serie (115200):
    e  estado (memoria, pendientes, WiFi, sensores)
    m  leer los sensores ahora y mostrarlos (NO se guarda ni se manda)
    w  cortar / volver a conectar el WiFi, para probar sin tocar el router
    B  borrar todo el registro (mayuscula, para no hacerlo sin querer)
*/

#include <Wire.h>
#include <time.h>
#include <Medicion.h>
#include <LineaServidor.h>
#include "config.h"
#include "sensores.h"
#include "anemometro.h"
#include "registro.h"
#include "red.h"
#include "envio.h"
#include "pantalla.h"

// Las mediciones caen en "turnos" de INTERVALO_MEDICION_S contados desde 1970: con 5 minutos,
// el turno cambia a las 10:00, 10:05, 10:10... Se mide una vez por turno.
static uint32_t turnoUltimaMedicion = 0;
static bool hayHoraAntes = false;
static unsigned long ultimoAvisoSinHora = 0;

static void imprimirMedicion(const char* titulo, const Medicion& m) {
  char t[16], h[16], v[16], p[16], g[16];
  formatearDato(t, sizeof(t), m.temperatura, m.estadoTemperatura, DECIMALES_TEMPERATURA);
  formatearDato(h, sizeof(h), m.humedad, m.estadoHumedad, DECIMALES_HUMEDAD);
  formatearDato(v, sizeof(v), m.velocidad, m.estadoVelocidad, DECIMALES_VELOCIDAD);
  formatearDato(p, sizeof(p), m.presion, m.estadoPresion, DECIMALES_PRESION);
  formatearDato(g, sizeof(g), m.gas, m.estadoGas, DECIMALES_GAS);

  time_t momento = (time_t)m.marcaTiempo;
  tm f;
  localtime_r(&momento, &f);
  Serial.printf("%s %02d:%02d  temp %s C  hum %s %%  viento %s km/h  presion %s hPa"
                "  CO %s ppm  (direccion, luz: NaN)\n",
                titulo, f.tm_hour, f.tm_min, t, h, v, p, g);
}

static void medirYGuardar(time_t ahora) {
  Medicion medicion = medicionVacia((uint32_t)ahora);
  leerSensores(medicion, true);
  anotarViento(medicion, true);

  imprimirMedicion("[medicion]", medicion);
  if (guardarMedicion(medicion)) {
    Serial.printf("           -> guardada en memoria (pendientes: %lu)\n",
                  (unsigned long)cantidadPendientes(DESTINO_SHEETS));
  } else {
    Serial.println("           -> ERROR: no se pudo guardar en la memoria");
  }
  pantallaMedicion(medicion);
}

static void imprimirInstalado(const char* nombre, bool instalado) {
  Serial.printf("  %-11s %s\n", nombre, instalado ? "instalado" : "NO instalado (NaN)");
}

static void mostrarEstado() {
  Serial.println();
  Serial.println("--- estado ---");
  time_t ahora;
  if (obtenerHora(ahora)) {
    tm f;
    localtime_r(&ahora, &f);
    Serial.printf("hora: %02d/%02d/%04d %02d:%02d:%02d\n", f.tm_mday, f.tm_mon + 1,
                  f.tm_year + 1900, f.tm_hour, f.tm_min, f.tm_sec);
  } else {
    Serial.println("hora: SIN HORA (no se mide hasta que llegue por NTP o del DS3231)");
  }
  Serial.printf("WiFi: %s\n", textoEstadoWifi());
  Serial.printf("una medicion cada %lu s\n", (unsigned long)INTERVALO_MEDICION_S);
  Serial.printf("memoria: %lu guardadas en total\n", (unsigned long)cantidadEscritas());
  for (int d = 0; d < CANTIDAD_DESTINOS; d++) {
    Destino destino = (Destino)d;
    if (!destinoActivo(destino)) {
      Serial.printf("  %-9s desactivado en config.h\n", nombreDestino(destino));
      continue;
    }
    Serial.printf("  %-9s pendientes: %lu   perdidas: %lu   ultimo envio: %s\n",
                  nombreDestino(destino), (unsigned long)cantidadPendientes(destino),
                  (unsigned long)cantidadPerdidas(destino),
                  ultimoEnvioFallo(destino) ? "FALLO"
                  : ultimoEnvioOk(destino) != 0 ? "OK" : "todavia ninguno");
  }
  if (destinoActivo(DESTINO_ITR)) {
    Serial.printf("  rechazadas por el ITR: %lu\n", (unsigned long)rechazadasItr());
  }
  Serial.println("sensores:");
  imprimirInstalado("DHT22", DHT22_INSTALADO);
  imprimirInstalado("anemometro", ANEMOMETRO_INSTALADO);
  imprimirInstalado("BMP180", BMP180_INSTALADO);
  imprimirInstalado("MQ-7 (CO)", MQ7_INSTALADO);
  imprimirInstalado("pantalla", PANTALLA_INSTALADA);
  imprimirInstalado("DS3231", RTC_INSTALADO);
  Serial.println("--------------");
  Serial.println();
}

// Lectura de prueba: no se guarda ni se manda. Sirve para ver un sensor recien conectado
// (o desconectado a proposito) sin esperar al proximo turno.
static void lecturaDePrueba() {
  time_t ahora;
  obtenerHora(ahora);
  Medicion medicion = medicionVacia((uint32_t)ahora);
  leerSensores(medicion, true);
  anotarViento(medicion, false);  // solo mira: no corta el promedio en curso
  imprimirMedicion(">>> lectura de prueba (NO se guarda):", medicion);
  if (ANEMOMETRO_INSTALADO) {
    Serial.printf("    pin del anemometro: %s (si nunca cambia al girar la rueda, revisar el"
                  " cable de GPIO18)\n", nivelPinEncoder() ? "ALTO" : "BAJO");
  }
}

static void mostrarAyuda() {
  Serial.println("Teclas: e estado   m leer sensores ahora   w cortar/reconectar WiFi"
                 "   B borrar registro");
}

static void leerTeclas() {
  if (Serial.available() <= 0) return;
  char tecla = Serial.read();

  if (tecla == 'e') {
    mostrarEstado();
  } else if (tecla == 'm') {
    lecturaDePrueba();
  } else if (tecla == 'w') {
    alternarWifi();
  } else if (tecla == 'B') {
    if (borrarRegistro()) {
      Serial.println(">>> registro BORRADO.");
    } else {
      Serial.println(">>> ERROR: no se pudo borrar el registro");
    }
  } else if (tecla != '\r' && tecla != '\n') {
    mostrarAyuda();
  }
}

// Decide si toca medir. Sin hora no se mide: no se sabria cuando fue.
static void atenderMedicion() {
  time_t ahora;
  if (!obtenerHora(ahora)) {
    if (millis() - ultimoAvisoSinHora >= INTERVALO_AVISO_WIFI_MS) {
      ultimoAvisoSinHora = millis();
      Serial.println("[medicion] sin hora todavia: no se mide hasta que llegue por internet"
                     " (hace falta WiFi una vez, o el DS3231)");
    }
    return;
  }

  uint32_t turno = (uint32_t)ahora / INTERVALO_MEDICION_S;
  if (!hayHoraAntes) {
    // Primera vez con hora: se espera al proximo turno, asi las mediciones caen en horas
    // redondas y un reinicio no repite el minuto de la medicion anterior.
    hayHoraAntes = true;
    turnoUltimaMedicion = turno;
    descartarPeriodoViento();
    time_t proxima = (time_t)(turno + 1) * INTERVALO_MEDICION_S;
    tm f;
    localtime_r(&proxima, &f);
    Serial.printf("[medicion] primera medicion a las %02d:%02d (tecla m para ver los sensores"
                  " ya)\n\n", f.tm_hour, f.tm_min);
    return;
  }
  if (turno != turnoUltimaMedicion) {
    turnoUltimaMedicion = turno;
    medirYGuardar(ahora);
  }
}

void setup() {
  Serial.begin(VELOCIDAD_SERIE);
  delay(500);  // le da tiempo al puerto USB a levantarse antes del primer mensaje

  Serial.println();
  Serial.println("=== Estacion meteorologica - firmware final ===");
  Serial.printf("Estacion %s, una medicion cada %lu s\n", CODIGO_ESTACION,
                (unsigned long)INTERVALO_MEDICION_S);
  Serial.printf("Envio: planilla %s, ITR %s\n", ENVIAR_A_SHEETS ? "SI" : "no",
                ENVIAR_A_ITR ? "SI" : "no (desactivado)");
  if (ANEMOMETRO_INSTALADO) Serial.println("Anemometro SIN CALIBRAR: los km/h son aproximados.");
  mostrarAyuda();
  Serial.println();

  // Bus I2C compartido: BMP180, pantalla y DS3231.
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, FRECUENCIA_I2C_HZ);

  iniciarPantalla();
  iniciarSensores();
  iniciarAnemometro();

  if (!iniciarRegistro()) {
    // Sin memoria no hay estacion: no se puede garantizar que no se pierdan datos.
    Serial.println("Sin memoria no hay registro: la estacion queda detenida.");
    while (true) delay(1000);
  }

  iniciarRed();
}

void loop() {
  leerTeclas();
  atenderRed();
  atenderMedicion();  // primero medir: no depende de la red
  atenderEnvios();    // despues, si se puede, mandar
  atenderPantalla();
}
