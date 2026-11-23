#include "red.h"
#include "config.h"
#include "secretos.h"  // si da error aca: falta copiar secretos.h (ver LEEME.md)
#include "rtc.h"
#include <WiFi.h>
#include <esp_sntp.h>
#include <sys/time.h>

static bool habilitado = true;
static bool estabaConectado = false;
static unsigned long ultimoAvisoWifi = 0;
static bool horaAvisada = false;

// La avisa el sistema (desde otra tarea) cada vez que llega la hora por NTP.
static volatile bool llegoHoraNtp = false;
static void alSincronizarNtp(struct timeval*) { llegoHoraNtp = true; }

static void conectarWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_NOMBRE, WIFI_CLAVE);
}

static void imprimirHora(const char* titulo, time_t momento) {
  tm f;
  localtime_r(&momento, &f);
  Serial.printf("%s %02d/%02d/%04d %02d:%02d:%02d\n", titulo, f.tm_mday, f.tm_mon + 1,
                f.tm_year + 1900, f.tm_hour, f.tm_min, f.tm_sec);
}

static void tomarHoraDelRtc() {
  if (!RTC_INSTALADO) return;
  time_t hora;
  if (!rtcResponde()) {
    Serial.println("[rtc] FALLA DS3231: no responde en 0x68. Revisar SDA, SCL y 3V3");
    return;
  }
  if (!leerRtc(hora)) {
    Serial.println("[rtc] el DS3231 perdio la hora (pila?): se espera a NTP");
    return;
  }
  timeval tv = {hora, 0};
  settimeofday(&tv, nullptr);
  imprimirHora("[rtc] hora tomada del DS3231:", hora);
}

void iniciarRed() {
  // La zona horaria y el pedido de hora a internet; se completa cuando haya WiFi.
  configTzTime(ZONA_HORARIA, SERVIDOR_NTP_1, SERVIDOR_NTP_2);
  sntp_set_time_sync_notification_cb(alSincronizarNtp);
  tomarHoraDelRtc();
  conectarWifi();
}

void atenderRed() {
  bool conectado = WiFi.status() == WL_CONNECTED;
  if (conectado && !estabaConectado) {
    Serial.print("[wifi] conectado. IP: ");
    Serial.print(WiFi.localIP());
    Serial.printf("  senal: %d dBm\n", WiFi.RSSI());
  } else if (!conectado && estabaConectado) {
    Serial.println("[wifi] se perdio la conexion; se sigue midiendo y guardando");
  }
  if (!conectado && habilitado && millis() - ultimoAvisoWifi >= INTERVALO_AVISO_WIFI_MS) {
    ultimoAvisoWifi = millis();
    Serial.printf("[wifi] esperando conexion a \"%s\"...\n", WIFI_NOMBRE);
  }
  estabaConectado = conectado;

  if (llegoHoraNtp) {
    llegoHoraNtp = false;
    time_t ahora = time(nullptr);
    // La primera vez se avisa; despues NTP resincroniza cada hora en silencio.
    if (!horaAvisada) imprimirHora("[ntp] hora sincronizada:", ahora);
    horaAvisada = true;
    if (RTC_INSTALADO && !escribirRtc(ahora)) {
      Serial.println("[rtc] FALLA DS3231: no se pudo poner en hora");
    }
  }
}

bool wifiConectado() { return WiFi.status() == WL_CONNECTED; }

void alternarWifi() {
  if (habilitado) {
    habilitado = false;
    WiFi.setAutoReconnect(false);
    WiFi.disconnect();
    Serial.println(">>> WiFi CORTADO a mano. Las mediciones se van a acumular en memoria.");
    Serial.println("    Tecla w de nuevo para reconectar.");
  } else {
    habilitado = true;
    conectarWifi();
    Serial.println(">>> reconectando el WiFi; las pendientes se mandan solas al conectar.");
  }
}

const char* textoEstadoWifi() {
  if (wifiConectado()) return "conectado";
  return habilitado ? "sin conexion" : "cortado a mano";
}

bool obtenerHora(time_t& ahora) {
  ahora = time(nullptr);
  tm f;
  localtime_r(&ahora, &f);
  return f.tm_year + 1900 >= ANIO_MINIMO_VALIDO;  // antes de esto: el reloj no esta en hora
}
