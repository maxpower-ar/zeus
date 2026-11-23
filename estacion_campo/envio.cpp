#include "envio.h"
#include "config.h"
#include "registro.h"
#include "red.h"
#include "envio_sheets.h"
#include "envio_itr.h"

struct EstadoEnvio {
  bool fallo;                  // el ultimo intento fallo: esperar antes de reintentar
  unsigned long momentoFallo;  // millis() del ultimo fallo
  time_t ultimoOk;             // hora del ultimo envio confirmado (0 = ninguno todavia)
};

static EstadoEnvio estados[CANTIDAD_DESTINOS] = {};
static int siguienteDestino = 0;
static uint32_t rechazadas = 0;

// static: 20 mediciones son 800 bytes, mejor no ponerlas en la pila del loop.
static Medicion lote[LOTE_MAXIMO_SHEETS];

static bool listoParaEnviar(Destino destino) {
  if (!destinoActivo(destino)) return false;
  if (cantidadPendientes(destino) == 0) return false;
  const EstadoEnvio& e = estados[destino];
  return !(e.fallo && millis() - e.momentoFallo < INTERVALO_REINTENTO_MS);
}

static void anotarResultado(Destino destino, bool ok) {
  EstadoEnvio& e = estados[destino];
  e.fallo = !ok;
  if (ok) {
    e.ultimoOk = time(nullptr);
    Serial.printf("  -> listo. Quedan pendientes para %s: %lu\n\n", nombreDestino(destino),
                  (unsigned long)cantidadPendientes(destino));
  } else {
    e.momentoFallo = millis();
    Serial.printf("  -> FALLO. Siguen en memoria; reintento en %lu s\n\n",
                  INTERVALO_REINTENTO_MS / 1000);
  }
}

// Hay pendientes pero no se pudo leer ninguna (falla de la memoria): se cuenta como envio
// fallido, asi se espera INTERVALO_REINTENTO_MS en vez de reintentar en cada vuelta del loop.
static bool hayLectura(Destino destino, int cantidad) {
  if (cantidad > 0) return true;
  if (cantidadPendientes(destino) > 0) {
    Serial.printf("[envio] %s: ERROR leyendo las pendientes de la memoria\n",
                  nombreDestino(destino));
    anotarResultado(destino, false);
  }
  return false;
}

static void enviarTandaSheets() {
  int cantidad = leerPendientes(DESTINO_SHEETS, lote, LOTE_MAXIMO_SHEETS);
  if (!hayLectura(DESTINO_SHEETS, cantidad)) return;
  Serial.printf("[envio] planilla: mandando %d de %lu pendientes:\n", cantidad,
                (unsigned long)cantidadPendientes(DESTINO_SHEETS));

  bool ok = enviarLoteSheets(lote, cantidad);
  if (ok) {
    Serial.println("  CONFIRMADAS por el servidor");
    marcarEnviadas(DESTINO_SHEETS, cantidad);
  }
  anotarResultado(DESTINO_SHEETS, ok);
}

static void enviarTandaItr() {
  if (!hayLectura(DESTINO_ITR, leerPendientes(DESTINO_ITR, lote, 1))) return;
  Serial.printf("[envio] ITR: mandando 1 de %lu pendientes:\n",
                (unsigned long)cantidadPendientes(DESTINO_ITR));

  ResultadoItr resultado = enviarMedicionItr(lote[0]);
  if (resultado == ITR_ACEPTADA) Serial.println("  CONFIRMADA por el ITR");
  if (resultado == ITR_RECHAZADA) {
    // Reintentarla no sirve (el servidor no la acepta asi): se saltea. La planilla la tiene.
    rechazadas++;
    Serial.println("  AVISO: el ITR RECHAZO esta medicion; se saltea (queda en la planilla)");
  }
  bool avanza = resultado != ITR_FALLO;
  if (avanza) marcarEnviadas(DESTINO_ITR, 1);
  anotarResultado(DESTINO_ITR, avanza);
}

void atenderEnvios() {
  if (!wifiConectado()) return;

  for (int i = 0; i < CANTIDAD_DESTINOS; i++) {
    Destino destino = (Destino)((siguienteDestino + i) % CANTIDAD_DESTINOS);
    if (!listoParaEnviar(destino)) continue;

    // La proxima vuelta empieza por el otro servidor.
    siguienteDestino = (destino + 1) % CANTIDAD_DESTINOS;
    if (destino == DESTINO_SHEETS) {
      enviarTandaSheets();
    } else {
      enviarTandaItr();
    }
    return;  // una tanda por vuelta: el loop sigue atendiendo teclas, mediciones y pantalla
  }
}

time_t ultimoEnvioOk(Destino destino) { return estados[destino].ultimoOk; }
bool ultimoEnvioFallo(Destino destino) { return estados[destino].fallo; }
uint32_t rechazadasItr() { return rechazadas; }
