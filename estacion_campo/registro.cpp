#include "registro.h"
#include "config.h"
#include <LittleFS.h>

static const char* const RUTA_REGISTRO = "/mediciones.bin";

// Firma para reconocer el archivo y version del formato. Si cambia el struct Medicion o la
// cabecera, se sube la version y el registro viejo se descarta en vez de leerse mal.
//   "EMRG" = el de la prueba pruebas/registro_y_envio (un solo destino)
//   "EMES" = el de este firmware (un contador de envio por destino). Firmas distintas: si se
//            sube la prueba y despues el firmware (o al reves), cada uno reconoce que el
//            archivo no es suyo y empieza de cero, en vez de leer contadores cruzados.
// Versiones: 2 = Medicion con estado por campo (VERSION_MEDICION 2).
static const uint32_t FIRMA = 0x454D4553;  // "EMES"
static const uint16_t VERSION_FORMATO = 2;
static_assert(VERSION_MEDICION == 2, "Cambio Medicion: subir VERSION_FORMATO y este control");

struct Cabecera {
  uint32_t firma;
  uint16_t version;
  uint16_t tamanoRegistro;
  uint32_t capacidad;
  uint32_t escritas;
  uint32_t enviadas[CANTIDAD_DESTINOS];
  uint32_t perdidas[CANTIDAD_DESTINOS];
};

// Cada medicion va con un CRC: si la flash se corrompe, la medicion se descarta en vez de
// mandarse con datos basura.
struct RegistroGuardado {
  Medicion medicion;
  uint8_t crc;
};

static Cabecera cabecera;

// CRC-8 (polinomio 0x07): una "huella" de los bytes. Si cambia un solo bit, cambia el CRC.
static uint8_t calcularCrc(const uint8_t* datos, size_t largo) {
  const uint8_t POLINOMIO = 0x07;
  uint8_t crc = 0;
  for (size_t i = 0; i < largo; i++) {
    crc ^= datos[i];
    for (int bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ POLINOMIO) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

static size_t posicionDe(uint32_t numero) {
  return sizeof(Cabecera) + (size_t)(numero % cabecera.capacidad) * sizeof(RegistroGuardado);
}

// Un destino desactivado no acumula pendientes: se lo deja siempre "al dia".
static void ponerAlDiaDestinosInactivos() {
  for (int d = 0; d < CANTIDAD_DESTINOS; d++) {
    if (!destinoActivo((Destino)d)) cabecera.enviadas[d] = cabecera.escritas;
  }
}

static bool escribirCabecera(File& archivo) {
  archivo.seek(0);
  return archivo.write((const uint8_t*)&cabecera, sizeof(cabecera)) == sizeof(cabecera);
}

static bool guardarCabecera() {
  File archivo = LittleFS.open(RUTA_REGISTRO, "r+");
  if (!archivo) return false;
  bool ok = escribirCabecera(archivo);
  archivo.close();
  return ok;
}

static bool crearRegistroNuevo() {
  memset(&cabecera, 0, sizeof(cabecera));
  cabecera.firma = FIRMA;
  cabecera.version = VERSION_FORMATO;
  cabecera.tamanoRegistro = sizeof(RegistroGuardado);
  cabecera.capacidad = CAPACIDAD_REGISTROS;

  File archivo = LittleFS.open(RUTA_REGISTRO, "w");
  if (!archivo) return false;
  bool ok = escribirCabecera(archivo);
  archivo.close();
  return ok;
}

bool iniciarRegistro() {
  // true = si la particion no tiene formato (primera vez), formatearla.
  if (!LittleFS.begin(true)) {
    Serial.println("[registro] ERROR: no se pudo montar LittleFS. Revisar el esquema de");
    Serial.println("           particiones (Herramientas -> Partition Scheme).");
    return false;
  }
  Serial.printf("[registro] memoria: %u KB usados de %u KB\n",
                (unsigned)(LittleFS.usedBytes() / 1024), (unsigned)(LittleFS.totalBytes() / 1024));

  File archivo = LittleFS.open(RUTA_REGISTRO, "r");
  if (archivo) {
    bool leida = archivo.read((uint8_t*)&cabecera, sizeof(cabecera)) == sizeof(cabecera);
    archivo.close();
    bool compatible = leida && cabecera.firma == FIRMA && cabecera.version == VERSION_FORMATO &&
                      cabecera.tamanoRegistro == sizeof(RegistroGuardado) &&
                      cabecera.capacidad == CAPACIDAD_REGISTROS;
    if (compatible) {
      ponerAlDiaDestinosInactivos();
      guardarCabecera();
      Serial.printf("[registro] registro existente: %lu guardadas, pendientes: planilla %lu,"
                    " ITR %lu\n",
                    (unsigned long)cabecera.escritas,
                    (unsigned long)cantidadPendientes(DESTINO_SHEETS),
                    (unsigned long)cantidadPendientes(DESTINO_ITR));
      return true;
    }
    Serial.println("[registro] AVISO: el registro guardado es de otro formato (o de una");
    Serial.println("           prueba); se empieza de cero");
  }

  if (!crearRegistroNuevo()) {
    Serial.println("[registro] ERROR: no se pudo crear el archivo del registro");
    return false;
  }
  Serial.println("[registro] registro nuevo creado");
  return true;
}

bool guardarMedicion(const Medicion& medicion) {
  RegistroGuardado registro;
  registro.medicion = medicion;
  registro.crc = calcularCrc((const uint8_t*)&registro.medicion, sizeof(registro.medicion));

  File archivo = LittleFS.open(RUTA_REGISTRO, "r+");
  if (!archivo) return false;

  // Mientras el archivo no se lleno, la posicion nueva es justo el final: se agrega.
  // Despues, cae sobre la casilla de la medicion mas vieja: se pisa.
  archivo.seek(posicionDe(cabecera.escritas));
  bool ok = archivo.write((const uint8_t*)&registro, sizeof(registro)) == sizeof(registro);

  if (ok) {
    cabecera.escritas++;
    for (int d = 0; d < CANTIDAD_DESTINOS; d++) {
      if (cabecera.escritas - cabecera.enviadas[d] > cabecera.capacidad) {
        // Se piso una que nunca llego a ese servidor.
        cabecera.enviadas[d] = cabecera.escritas - cabecera.capacidad;
        cabecera.perdidas[d]++;
      }
    }
    ponerAlDiaDestinosInactivos();
    // Medicion y cabecera se escriben en la misma apertura del archivo: LittleFS confirma
    // los cambios al cerrarlo, asi que un corte de luz no deja una sin la otra.
    ok = escribirCabecera(archivo);
  }
  archivo.close();
  return ok;
}

int leerPendientes(Destino servidor, Medicion* destino, int maximo) {
  File archivo = LittleFS.open(RUTA_REGISTRO, "r");
  if (!archivo) return 0;

  int leidas = 0;
  bool cabeceraCambio = false;

  while (leidas < maximo && cantidadPendientes(servidor) > (uint32_t)leidas) {
    RegistroGuardado registro;
    archivo.seek(posicionDe(cabecera.enviadas[servidor] + leidas));
    bool ok = archivo.read((uint8_t*)&registro, sizeof(registro)) == sizeof(registro) &&
              registro.crc == calcularCrc((const uint8_t*)&registro.medicion,
                                          sizeof(registro.medicion));
    if (!ok) {
      if (leidas == 0) {
        // La mas vieja esta corrupta: se saltea para que no trabe a todas las demas.
        Serial.println("[registro] AVISO: medicion corrupta descartada (CRC incorrecto)");
        cabecera.enviadas[servidor]++;
        cabecera.perdidas[servidor]++;
        cabeceraCambio = true;
        continue;
      }
      break;  // se manda lo leido hasta aca; la corrupta queda primera en el proximo lote
    }
    destino[leidas++] = registro.medicion;
  }
  archivo.close();

  if (cabeceraCambio) guardarCabecera();
  return leidas;
}

bool marcarEnviadas(Destino servidor, int cantidad) {
  if (cantidad <= 0) return true;
  if ((uint32_t)cantidad > cantidadPendientes(servidor)) {
    cantidad = (int)cantidadPendientes(servidor);
  }
  cabecera.enviadas[servidor] += cantidad;
  return guardarCabecera();
}

uint32_t cantidadPendientes(Destino servidor) {
  return cabecera.escritas - cabecera.enviadas[servidor];
}
uint32_t cantidadPerdidas(Destino servidor) { return cabecera.perdidas[servidor]; }
uint32_t cantidadEscritas() { return cabecera.escritas; }

bool borrarRegistro() {
  LittleFS.remove(RUTA_REGISTRO);
  return crearRegistroNuevo();
}
