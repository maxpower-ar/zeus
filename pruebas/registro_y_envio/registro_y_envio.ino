/*
  PRUEBA DE CONCEPTO - Guardar primero, mandar despues

  Cada minuto mide (DHT22 + anemometro), GUARDA la medicion en la memoria flash y recien
  despues intenta mandarla a la planilla de Google. Si no hay WiFi, o Google no responde,
  la medicion queda pendiente en la memoria y se manda sola cuando vuelve la conexion:
  primero las mas viejas, en tandas de hasta 20.

  Una medicion solo se da por enviada cuando el servidor contesta OK. La memoria sobrevive
  a un corte de luz: al volver, sigue desde donde quedo.

  Es el paso siguiente a pruebas/envio_sheets (que mide y manda, sin memoria).

  Antes de compilar:
    1. Copiar secretos.h de pruebas/envio_sheets a esta carpeta (es el mismo archivo)
    2. Herramientas -> Partition Scheme: uno que tenga SPIFFS (por ejemplo el que viene por
       defecto, "Default 4MB with spiffs"). LittleFS usa esa particion.
    3. Librerias: "DHT sensor library" (Adafruit). LittleFS, WiFi y HTTPClient vienen con
       el soporte de ESP32. EstacionComun viene en el repositorio.

  Conexionado:
    DHT22:  DATA -> GPIO4, VCC -> 3V3
    Encoder del anemometro (FC-03): DO -> GPIO18, VCC -> 3V3 (NO a 5 V)
  Cualquiera de los dos puede faltar: el DHT22 sale como ERR y el anemometro como 0 km/h
  (desde el codigo, "quieto" y "desconectado" se ven iguales).

  Teclas en el monitor serie (115200):
    e  estado del registro (pendientes, enviadas, perdidas)
    w  cortar / volver a conectar el WiFi, para probar sin tocar el router
    B  borrar todo el registro (mayuscula, para no hacerlo sin querer)

  Datos que todavia no tienen sensor (gas, direccion, presion, luminosidad) van como NaN y
  quedan como celdas vacias en la planilla.
*/

#include <WiFi.h>
#include <time.h>
#include <DHT.h>
#include <Medicion.h>
#include "config.h"
#include "secretos.h"  // si da error aca: falta copiar secretos.h (ver arriba)
#include "registro.h"
#include "envio.h"

DHT dht(PIN_DHT, DHT22);

// ---------------------------------------------------------------- anemometro (interrupcion)
// Compartidas con la interrupcion: van volatile para que el bucle vea siempre el valor real.
volatile unsigned long pulsosTotales = 0;
volatile unsigned long ultimoPulsoUs = 0;

void IRAM_ATTR contarPulso() {
  unsigned long ahora = micros();
  if (ahora - ultimoPulsoUs < ANTIRREBOTE_US) return;  // rebote o ruido
  ultimoPulsoUs = ahora;
  pulsosTotales = pulsosTotales + 1;
}

unsigned long pulsosMedicionAnterior = 0;
unsigned long momentoMedicionAnterior = 0;

// Velocidad PROMEDIO desde la medicion anterior: es lo que informa una estacion
// meteorologica (el viento medio del periodo), no el valor de un instante.
float velocidadPromedioKmh(unsigned long ahoraMs) {
  noInterrupts();
  unsigned long total = pulsosTotales;
  interrupts();

  unsigned long pulsos = total - pulsosMedicionAnterior;
  float segundos = (ahoraMs - momentoMedicionAnterior) / 1000.0;
  pulsosMedicionAnterior = total;
  momentoMedicionAnterior = ahoraMs;

  if (segundos <= 0) return NAN;
  return (pulsos / segundos) * METROS_POR_PULSO * MS_A_KMH;
}

// ---------------------------------------------------------------- estado general
unsigned long ultimaMedicion = 0;
unsigned long ultimoIntentoFallido = 0;
bool huboFalloEnvio = false;
unsigned long ultimoAvisoWifi = 0;
bool estabaConectado = false;
bool wifiHabilitado = true;
bool horaAvisada = false;

void conectarWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_NOMBRE, WIFI_CLAVE);
}

// Muestra los cambios de estado del WiFi. La reconexion la hace el ESP32 solo.
void vigilarWifi() {
  bool conectado = WiFi.status() == WL_CONNECTED;
  if (conectado && !estabaConectado) {
    Serial.print("[wifi] conectado. IP: ");
    Serial.print(WiFi.localIP());
    Serial.printf("  senal: %d dBm\n", WiFi.RSSI());
  } else if (!conectado && estabaConectado) {
    Serial.println("[wifi] se perdio la conexion; se sigue midiendo y guardando");
  }
  if (!conectado && wifiHabilitado && millis() - ultimoAvisoWifi >= INTERVALO_AVISO_WIFI_MS) {
    ultimoAvisoWifi = millis();
    Serial.printf("[wifi] esperando conexion a \"%s\"...\n", WIFI_NOMBRE);
  }
  estabaConectado = conectado;
}

// Devuelve false mientras el reloj no este en hora (NTP todavia no sincronizo).
bool obtenerHora(tm& fechaHora) {
  if (!getLocalTime(&fechaHora, 10)) return false;
  return fechaHora.tm_year + 1900 >= ANIO_MINIMO_VALIDO;
}

void mostrarEstado() {
  Serial.printf("[registro] guardadas: %lu   pendientes: %lu   perdidas: %lu   WiFi: %s\n",
                (unsigned long)cantidadEscritas(), (unsigned long)cantidadPendientes(),
                (unsigned long)cantidadPerdidas(),
                WiFi.status() == WL_CONNECTED ? "conectado"
                : wifiHabilitado              ? "sin conexion"
                                              : "cortado a mano");
}

void medirYGuardar() {
  tm fechaHora;
  if (!obtenerHora(fechaHora)) {
    // Sin hora no hay medicion util: no se sabria cuando fue. Pasa solo si la placa arranco
    // sin internet; el DS3231 (todavia no comprado) es lo que va a resolver este caso.
    Serial.println("[medicion] sin hora todavia (hace falta WiFi una vez): no se registra");
    velocidadPromedioKmh(millis());  // se descarta este periodo del anemometro
    return;
  }

  Medicion medicion = medicionVacia((uint32_t)time(nullptr));
  // DHT22 y anemometro estan instalados: un NaN de la libreria es una falla (ERR).
  anotarValor(medicion.temperatura, medicion.estadoTemperatura, dht.readTemperature());
  anotarValor(medicion.humedad, medicion.estadoHumedad, dht.readHumidity());
  anotarValor(medicion.velocidad, medicion.estadoVelocidad, velocidadPromedioKmh(millis()));
  // gas, direccion, presion y luminosidad: sin sensor todavia, quedan en NaN.

  if (guardarMedicion(medicion)) {
    Serial.printf("[medicion] %02d:%02d  %.1f C  %.0f %%  %.1f km/h  -> guardada en memoria"
                  " (pendientes: %lu)\n",
                  fechaHora.tm_hour, fechaHora.tm_min, medicion.temperatura, medicion.humedad,
                  medicion.velocidad, (unsigned long)cantidadPendientes());
  } else {
    Serial.println("[medicion] ERROR: no se pudo guardar en la memoria");
  }
}

// Manda una tanda de pendientes, si hay WiFi y no estamos esperando un reintento.
void enviarPendientes() {
  if (cantidadPendientes() == 0) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (huboFalloEnvio && millis() - ultimoIntentoFallido < INTERVALO_REINTENTO_MS) return;

  Medicion lote[LOTE_MAXIMO];
  int cantidad = leerPendientes(lote, LOTE_MAXIMO);
  if (cantidad == 0) return;

  Serial.printf("[envio] mandando %d de %lu pendientes:\n", cantidad,
                (unsigned long)cantidadPendientes());

  if (enviarLote(lote, cantidad)) {
    marcarEnviadas(cantidad);
    huboFalloEnvio = false;
    Serial.printf("  -> CONFIRMADAS. Quedan pendientes: %lu\n\n",
                  (unsigned long)cantidadPendientes());
  } else {
    huboFalloEnvio = true;
    ultimoIntentoFallido = millis();
    Serial.printf("  -> FALLO. Siguen en memoria; reintento en %lu s\n\n",
                  INTERVALO_REINTENTO_MS / 1000);
  }
}

void leerTeclas() {
  if (Serial.available() <= 0) return;
  char tecla = Serial.read();

  if (tecla == 'e') {
    mostrarEstado();
  } else if (tecla == 'w') {
    if (wifiHabilitado) {
      wifiHabilitado = false;
      WiFi.setAutoReconnect(false);
      WiFi.disconnect();
      Serial.println(">>> WiFi CORTADO a mano. Las mediciones se van a acumular en memoria.");
      Serial.println("    Tecla w de nuevo para reconectar.");
    } else {
      wifiHabilitado = true;
      conectarWifi();
      Serial.println(">>> reconectando el WiFi; las pendientes se mandan solas al conectar.");
    }
  } else if (tecla == 'B') {
    borrarRegistro();
    Serial.println(">>> registro BORRADO.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("=== Registro y envio: guardar primero, mandar despues ===");
  Serial.printf("Estacion %s, una medicion cada %lu s\n", CODIGO_ESTACION,
                INTERVALO_MEDICION_MS / 1000);
  Serial.println("Teclas: e estado   w cortar/reconectar WiFi   B borrar registro");
  Serial.println("Anemometro SIN CALIBRAR: los km/h son aproximados.");
  Serial.println();

  dht.begin();

  // INPUT_PULLUP: la salida del LM393 es de colector abierto. FALLING: un pulso por ranura.
  pinMode(PIN_ENCODER, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER), contarPulso, FALLING);
  momentoMedicionAnterior = millis();

  if (!iniciarRegistro()) {
    Serial.println("Sin memoria no hay registro: se detiene la prueba.");
    while (true) delay(1000);
  }

  conectarWifi();
  configTzTime(ZONA_HORARIA, SERVIDOR_NTP_1, SERVIDOR_NTP_2);
  ultimaMedicion = millis();  // la primera medicion se adelanta al sincronizar la hora
}

void loop() {
  leerTeclas();
  vigilarWifi();

  tm ahora;
  bool hayHora = obtenerHora(ahora);
  if (hayHora && !horaAvisada) {
    horaAvisada = true;
    Serial.printf("[ntp] hora sincronizada: %02d/%02d/%04d %02d:%02d\n\n", ahora.tm_mday,
                  ahora.tm_mon + 1, ahora.tm_year + 1900, ahora.tm_hour, ahora.tm_min);
    ultimaMedicion = millis() - INTERVALO_MEDICION_MS;  // primera medicion ya mismo
  }

  // Primero medir: no depende de la red. Despues, si se puede, mandar.
  if (millis() - ultimaMedicion >= INTERVALO_MEDICION_MS) {
    ultimaMedicion = millis();
    medirYGuardar();
  }

  enviarPendientes();
}
