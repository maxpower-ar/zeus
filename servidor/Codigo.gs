/**
 * Servidor de la Estacion Meteorologica - Google Apps Script
 *
 * Copia de referencia: el codigo que corre de verdad es el que esta pegado en el editor de
 * Apps Script de la planilla. Si se cambia alla, se actualiza este archivo y viceversa.
 * Instrucciones de instalacion en servidor/LEEME.md.
 *
 * doPost: recibe una o varias lineas del ESP32 (una por renglon) con el formato de
 *         docs/servidor.md y guarda cada una como fila en la pestana de su mes (AAAA-MM).
 * doGet:  devuelve las mediciones de un rango de fechas en JSON, para la pagina web.
 */

const ZONA_HORARIA = 'America/Argentina/Buenos_Aires';

const ENCABEZADOS = [
  'Fecha y hora', 'Codigo', 'Temperatura (°C)', 'Humedad (%)', 'Viento (km/h)', 'Gas',
  'Direccion', 'Presion (hPa)', 'Luminosidad (lux)', 'Recibido', 'Id'
];
const COLUMNA_ID = ENCABEZADOS.length;
const COLUMNA_RECIBIDO = COLUMNA_ID - 1;

// Las pestanas creadas antes del 2026-11-17 no tienen presion ni luminosidad: tenian
// 'Recibido' en la columna 8. Se migran solas la primera vez que se usan (ver obtenerPestanaMes).
const COLUMNA_RECIBIDO_VIEJA = 8;
const COLUMNAS_AGREGADAS = 2;

const FORMATO_FECHA_HORA = 'dd/MM/yyyy HH:mm';
const FORMATO_RECIBIDO = 'dd/MM/yyyy HH:mm:ss';
const FORMATO_PESTANA = 'yyyy-MM';

const ESPERA_BLOQUEO_MS = 20000;
const DIAS_POR_DEFECTO = 7;
const MAX_DIAS_CONSULTA = 92;
const MS_POR_DIA = 24 * 60 * 60 * 1000;

// [DD/MM/AAAA;HH:MM;CODIGO;temperatura;humedad;velocidad;gas;direccion;presion;luminosidad;EOF]
const PATRON_LINEA =
  /^\[(\d{2})\/(\d{2})\/(\d{4});(\d{2}):(\d{2});([A-Za-z0-9_]+);([^;\]]*);([^;\]]*);([^;\]]*);([^;\]]*);([^;\]]*);([^;\]]*);([^;\]]*);EOF\]$/;


// ---------------------------------------------------------------- recepcion

function doPost(e) {
  const claveGuardada = PropertiesService.getScriptProperties().getProperty('CLAVE_API');
  if (!claveGuardada) {
    return texto('ERROR falta la CLAVE_API: ejecutar generarClave() en el editor');
  }
  if (!e || !e.parameter || e.parameter.clave !== claveGuardada) {
    return texto('ERROR clave incorrecta');
  }

  const cuerpo = (e.postData && e.postData.contents) || '';
  const lineas = cuerpo.split(/\r?\n/).map(l => l.trim()).filter(l => l.length > 0);
  if (lineas.length === 0) {
    return texto('ERROR cuerpo vacio');
  }

  // Dos pedidos simultaneos podrian ver la misma linea como "nueva" y guardarla dos veces.
  // El bloqueo hace que se atiendan de a uno.
  const bloqueo = LockService.getScriptLock();
  if (!bloqueo.tryLock(ESPERA_BLOQUEO_MS)) {
    return texto('ERROR servidor ocupado, reintentar');
  }

  try {
    const libro = SpreadsheetApp.getActiveSpreadsheet();
    let guardadas = 0;
    let duplicadas = 0;
    const errores = [];

    lineas.forEach((linea, i) => {
      const resultado = guardarLinea(libro, linea);
      if (resultado === 'GUARDADA') guardadas++;
      else if (resultado === 'DUPLICADA') duplicadas++;
      else errores.push('linea ' + (i + 1) + ': ' + resultado);
    });
    SpreadsheetApp.flush();

    const resumen = 'guardadas=' + guardadas + ' duplicadas=' + duplicadas;
    if (errores.length > 0) {
      return texto('ERROR ' + errores.join(' | ') + ' (' + resumen + ')');
    }
    // Una duplicada tambien es OK: la medicion ya estaba guardada, el ESP32 puede darla por enviada.
    return texto('OK ' + resumen);
  } finally {
    bloqueo.releaseLock();
  }
}

/**
 * Guarda una linea. Devuelve 'GUARDADA', 'DUPLICADA' o la descripcion del error.
 */
function guardarLinea(libro, linea) {
  const m = PATRON_LINEA.exec(linea);
  if (!m) return 'formato invalido: ' + linea;

  // Se vuelve a formatear la fecha leida y se compara con la original: asi se rechazan
  // fechas imposibles como 31/02, que parseDate aceptaria corriendolas al mes siguiente.
  const textoFecha = m[1] + '/' + m[2] + '/' + m[3] + ' ' + m[4] + ':' + m[5];
  let fecha;
  try {
    fecha = Utilities.parseDate(textoFecha, ZONA_HORARIA, FORMATO_FECHA_HORA);
  } catch (err) {
    return 'fecha invalida: ' + textoFecha;
  }
  if (Utilities.formatDate(fecha, ZONA_HORARIA, FORMATO_FECHA_HORA) !== textoFecha) {
    return 'fecha invalida: ' + textoFecha;
  }

  const codigo = m[6];
  const numeros = [m[7], m[8], m[9], m[10]].map(convertirNumero);
  const direccion = (m[11] === 'NaN') ? '' : m[11];
  const presionYLuz = [m[12], m[13]].map(convertirNumero);
  if (numeros.concat(presionYLuz).some(n => n === undefined)) {
    return 'valor no numerico: ' + linea;
  }

  // La pestana sale de la fecha de la MEDICION, no de cuando llego: una medicion del 31/10
  // que llega el 1/11 despues de un corte de red va a la pestana de octubre.
  const hoja = obtenerPestanaMes(libro, Utilities.formatDate(fecha, ZONA_HORARIA, FORMATO_PESTANA));
  const id = codigo + '|' + Utilities.formatDate(fecha, ZONA_HORARIA, 'yyyy-MM-dd HH:mm');
  if (existeId(hoja, id)) return 'DUPLICADA';

  hoja.appendRow([fecha, codigo].concat(numeros, [direccion], presionYLuz, [new Date(), id]));
  return 'GUARDADA';
}

/**
 * "26,1" o "26.1" -> 26.1   "NaN" o vacio -> '' (celda vacia: dato inexistente, NO cero)
 * Cualquier otra cosa -> undefined (la linea se rechaza).
 */
function convertirNumero(textoValor) {
  const t = textoValor.trim();
  if (t === '' || t === 'NaN') return '';
  const n = Number(t.replace(',', '.'));
  return isFinite(n) ? n : undefined;
}

function obtenerPestanaMes(libro, nombre) {
  let hoja = libro.getSheetByName(nombre);
  if (hoja) {
    migrarPestanaVieja(hoja);
    return hoja;
  }

  hoja = libro.insertSheet(nombre, libro.getSheets().length);
  hoja.getRange(1, 1, 1, ENCABEZADOS.length).setValues([ENCABEZADOS]).setFontWeight('bold');
  hoja.setFrozenRows(1);
  hoja.getRange('A:A').setNumberFormat(FORMATO_FECHA_HORA);
  hoja.getRange(1, COLUMNA_RECIBIDO, hoja.getMaxRows(), 1).setNumberFormat(FORMATO_RECIBIDO);
  return hoja;
}

// Agrega las columnas de presion y luminosidad (vacias) a una pestana del formato viejo,
// antes de 'Recibido'. Las filas que ya estaban quedan con esas dos celdas vacias.
function migrarPestanaVieja(hoja) {
  if (hoja.getRange(1, COLUMNA_RECIBIDO_VIEJA).getValue() !== 'Recibido') return;
  hoja.insertColumnsBefore(COLUMNA_RECIBIDO_VIEJA, COLUMNAS_AGREGADAS);
  hoja.getRange(1, 1, 1, ENCABEZADOS.length).setValues([ENCABEZADOS]).setFontWeight('bold');
}

function existeId(hoja, id) {
  const ultimaFila = hoja.getLastRow();
  if (ultimaFila < 2) return false;
  return hoja.getRange(2, COLUMNA_ID, ultimaFila - 1, 1)
    .createTextFinder(id).matchEntireCell(true).findNext() !== null;
}

function texto(mensaje) {
  return ContentService.createTextOutput(mensaje).setMimeType(ContentService.MimeType.TEXT);
}


// ---------------------------------------------------------------- consulta (para la pagina)

/**
 * GET .../exec                                  -> ultimos 7 dias
 * GET .../exec?desde=2026-10-01&hasta=2026-10-31 -> ese rango (maximo 92 dias)
 *
 * Los datos faltantes vienen como null, nunca como 0.
 */
function doGet(e) {
  const p = (e && e.parameter) || {};
  let desde, hasta;
  try {
    hasta = p.hasta ? Utilities.parseDate(p.hasta + ' 23:59', ZONA_HORARIA, 'yyyy-MM-dd HH:mm')
                    : new Date();
    desde = p.desde ? Utilities.parseDate(p.desde + ' 00:00', ZONA_HORARIA, 'yyyy-MM-dd HH:mm')
                    : new Date(hasta.getTime() - DIAS_POR_DEFECTO * MS_POR_DIA);
  } catch (err) {
    return json({ error: 'fechas invalidas, usar AAAA-MM-DD' });
  }
  if (desde > hasta) return json({ error: 'desde es posterior a hasta' });
  if (hasta - desde > MAX_DIAS_CONSULTA * MS_POR_DIA) {
    return json({ error: 'rango maximo: ' + MAX_DIAS_CONSULTA + ' dias' });
  }

  const libro = SpreadsheetApp.getActiveSpreadsheet();
  const datos = [];
  pestanasEntre(desde, hasta).forEach(nombre => {
    const hoja = libro.getSheetByName(nombre);
    if (!hoja || hoja.getLastRow() < 2) return;
    // La migracion se hace solo en doPost, que tiene el bloqueo. Aca una pestana vieja se lee
    // tal cual: no tiene presion ni luminosidad.
    const vieja = hoja.getRange(1, COLUMNA_RECIBIDO_VIEJA).getValue() === 'Recibido';
    const filas = hoja.getRange(2, 1, hoja.getLastRow() - 1, ENCABEZADOS.length).getValues();
    filas.forEach(f => {
      const fecha = f[0];
      if (!(fecha instanceof Date) || fecha < desde || fecha > hasta) return;
      datos.push({
        fecha: Utilities.formatDate(fecha, ZONA_HORARIA, "yyyy-MM-dd'T'HH:mm"),
        codigo: f[1],
        temperatura: vacioANull(f[2]),
        humedad: vacioANull(f[3]),
        viento: vacioANull(f[4]),
        gas: vacioANull(f[5]),
        direccion: vacioANull(f[6]),
        presion: vieja ? null : vacioANull(f[7]),
        luminosidad: vieja ? null : vacioANull(f[8])
      });
    });
  });
  datos.sort((a, b) => (a.fecha < b.fecha ? -1 : a.fecha > b.fecha ? 1 : 0));

  return json({
    desde: Utilities.formatDate(desde, ZONA_HORARIA, "yyyy-MM-dd'T'HH:mm"),
    hasta: Utilities.formatDate(hasta, ZONA_HORARIA, "yyyy-MM-dd'T'HH:mm"),
    cantidad: datos.length,
    datos: datos
  });
}

/** Nombres AAAA-MM de todos los meses que toca el rango. */
function pestanasEntre(desde, hasta) {
  let anio = Number(Utilities.formatDate(desde, ZONA_HORARIA, 'yyyy'));
  let mes = Number(Utilities.formatDate(desde, ZONA_HORARIA, 'MM'));
  const ultima = Utilities.formatDate(hasta, ZONA_HORARIA, FORMATO_PESTANA);
  const nombres = [];
  while (true) {
    const nombre = anio + '-' + (mes < 10 ? '0' : '') + mes;
    nombres.push(nombre);
    if (nombre >= ultima) break;
    mes++;
    if (mes > 12) { mes = 1; anio++; }
  }
  return nombres;
}

function vacioANull(valor) {
  return valor === '' ? null : valor;
}

function json(objeto) {
  return ContentService.createTextOutput(JSON.stringify(objeto))
    .setMimeType(ContentService.MimeType.JSON);
}


// ---------------------------------------------------------------- herramientas (se ejecutan a mano)

/**
 * Crea la clave que el ESP32 tiene que mandar en cada envio y la guarda en las propiedades
 * del script (no queda escrita en el codigo). Si ya existe, solo la muestra.
 * Ver el resultado en "Registro de ejecucion".
 */
function generarClave() {
  const propiedades = PropertiesService.getScriptProperties();
  let clave = propiedades.getProperty('CLAVE_API');
  if (clave) {
    Logger.log('Ya existe una clave (no se cambio): ' + clave);
    return;
  }
  clave = Utilities.getUuid().replace(/-/g, '');
  propiedades.setProperty('CLAVE_API', clave);
  Logger.log('Clave creada. Copiarla en CLAVE_API de secretos.h: ' + clave);
}

/**
 * Prueba el guardado sin ESP32. Guarda una fila con codigo EM_PRUEBA y despues intenta
 * guardarla de nuevo: la segunda vez tiene que responder DUPLICADA.
 * Al terminar se pueden borrar a mano las filas EM_PRUEBA.
 */
function probarGuardado() {
  const ahora = Utilities.formatDate(new Date(), ZONA_HORARIA, 'dd/MM/yyyy;HH:mm');
  const linea = '[' + ahora + ';EM_PRUEBA;21,5;55;3,2;NaN;NaN;1013,2;NaN;EOF]';
  const libro = SpreadsheetApp.getActiveSpreadsheet();
  Logger.log(linea + '  ->  ' + guardarLinea(libro, linea));
  Logger.log('Segunda vez (tiene que decir DUPLICADA)  ->  ' + guardarLinea(libro, linea));
}
