// Panel de la estacion: lee la planilla con el doGet de servidor/Codigo.gs y la dibuja.
//
// Cada dato que devuelve el servidor puede ser:
//   un numero -> medicion valida
//   "ERR"     -> el sensor esta instalado pero fallo (se muestra en rojo)
//   null      -> el sensor no esta instalado (NaN en la planilla)

'use strict';

const ZONA = 'America/Argentina/Buenos_Aires';
const DESFASE_ARGENTINA = '-03:00';  // UTC-3 todo el ano, sin horario de verano
const MAX_DIAS_POR_CONSULTA = 90;    // el servidor acepta hasta 92 dias por pedido
const FILAS_POR_PAGINA = 50;
const MS_POR_DIA = 24 * 60 * 60 * 1000;
const HUECO_MAXIMO_MS = 20 * 60 * 1000;  // mas que esto sin datos: la linea se corta

const VARIABLES = [
  { clave: 'temperatura', nombre: 'Temperatura', unidad: '°C', decimales: 1, color: '#e4572e' },
  { clave: 'humedad', nombre: 'Humedad', unidad: '%', decimales: 0, color: '#2f6fdf' },
  { clave: 'viento', nombre: 'Viento', unidad: 'km/h', decimales: 1, color: '#17a398' },
  { clave: 'gas', nombre: 'CO', unidad: 'ppm', decimales: 0, color: '#8e5bd6' },
  { clave: 'presion', nombre: 'Presión', unidad: 'hPa', decimales: 1, color: '#d18b00' },
  { clave: 'luminosidad', nombre: 'Luminosidad', unidad: 'lux', decimales: 0, color: '#c94f9b' },
];

const estado = {
  rango: '1',          // '1', '7', '30', 'todo' o 'fechas'
  estacion: CONFIG.estacionPorDefecto,
  recientes: [],       // ultimos 7 dias, para "Ahora"
  historial: [],       // el periodo elegido
  pagina: 0,
  graficos: {},
};

const $ = id => document.getElementById(id);

// ------------------------------------------------------------------ valores

function tipoDe(valor) {
  if (typeof valor === 'number' && isFinite(valor)) return 'ok';
  if (valor === 'ERR') return 'err';
  return 'nan';  // null, '' o el campo ausente (version vieja del servidor)
}

function textoValor(valor, decimales) {
  const tipo = tipoDe(valor);
  if (tipo === 'ok') return valor.toFixed(decimales);
  return tipo === 'err' ? 'ERR' : 'NaN';
}

function fechaDe(fila) {
  return new Date(fila.fecha + DESFASE_ARGENTINA);
}

const formatoFechaHora = new Intl.DateTimeFormat('es-AR', {
  timeZone: ZONA, day: '2-digit', month: '2-digit', year: 'numeric', hour: '2-digit', minute: '2-digit',
});
const formatoCorto = new Intl.DateTimeFormat('es-AR', {
  timeZone: ZONA, day: '2-digit', month: '2-digit', hour: '2-digit', minute: '2-digit',
});
const formatoDia = new Intl.DateTimeFormat('en-CA', { timeZone: ZONA });  // AAAA-MM-DD

function diaArgentina(fecha) {
  return formatoDia.format(fecha);
}

// ------------------------------------------------------------------ servidor

async function consultarTramo(desde, hasta) {
  const url = new URL(CONFIG.urlDatos);
  if (desde) url.searchParams.set('desde', desde);
  if (hasta) url.searchParams.set('hasta', hasta);
  const respuesta = await fetch(url, { redirect: 'follow' });
  if (!respuesta.ok) throw new Error('el servidor respondio HTTP ' + respuesta.status);
  const json = await respuesta.json();
  if (json.error) throw new Error('el servidor dice: ' + json.error);
  return json.datos || [];
}

// El servidor da como mucho 92 dias por pedido: un periodo largo se pide en tramos.
async function consultar(desde, hasta) {
  const tramos = [];
  let inicio = new Date(desde + 'T00:00' + DESFASE_ARGENTINA);
  const fin = new Date(hasta + 'T00:00' + DESFASE_ARGENTINA);
  while (inicio <= fin) {
    const finTramo = new Date(Math.min(fin, inicio.getTime() + (MAX_DIAS_POR_CONSULTA - 1) * MS_POR_DIA));
    tramos.push([diaArgentina(inicio), diaArgentina(finTramo)]);
    inicio = new Date(finTramo.getTime() + MS_POR_DIA);
  }
  const partes = await Promise.all(tramos.map(([d, h]) => consultarTramo(d, h)));
  const vistas = new Set();
  return partes.flat().filter(f => {
    const clave = f.codigo + '|' + f.fecha;
    if (vistas.has(clave)) return false;
    vistas.add(clave);
    return true;
  }).sort((a, b) => (a.fecha < b.fecha ? -1 : a.fecha > b.fecha ? 1 : 0));
}

function periodoElegido() {
  const hoy = new Date();
  if (estado.rango === 'fechas') return [$('desde').value, $('hasta').value];
  if (estado.rango === 'todo') return [CONFIG.inicioHistorial, diaArgentina(hoy)];
  const dias = Number(estado.rango);
  return [diaArgentina(new Date(hoy - dias * MS_POR_DIA)), diaArgentina(hoy)];
}

// ------------------------------------------------------------------ dibujo

function filasDeEstacion(filas) {
  return filas.filter(f => f.codigo === estado.estacion);
}

function filasDelPeriodo() {
  let filas = filasDeEstacion(estado.historial);
  if (estado.rango !== 'fechas' && estado.rango !== 'todo') {
    const desde = Date.now() - Number(estado.rango) * MS_POR_DIA;
    filas = filas.filter(f => fechaDe(f).getTime() >= desde);
  }
  return filas;
}

function actualizarSelectorEstaciones() {
  const codigos = new Set([CONFIG.estacionPorDefecto]);
  estado.recientes.concat(estado.historial).forEach(f => codigos.add(f.codigo));
  const selector = $('estacion');
  const actuales = Array.from(selector.options).map(o => o.value);
  const lista = Array.from(codigos).sort();
  if (lista.join() === actuales.join()) return;
  selector.replaceChildren(...lista.map(c => new Option(c, c, false, c === estado.estacion)));
}

function dibujarEstado() {
  const caja = $('estado');
  const filas = filasDeEstacion(estado.recientes);
  caja.className = 'estado';
  if (filas.length === 0) {
    caja.classList.add('falla');
    $('estadoTexto').textContent = 'Sin mediciones en los últimos 7 días';
    return;
  }
  const ultima = fechaDe(filas[filas.length - 1]);
  const minutos = Math.round((Date.now() - ultima) / 60000);
  const hace = minutos < 1 ? 'recién' : minutos < 120 ? `hace ${minutos} min`
    : minutos < 2880 ? `hace ${Math.round(minutos / 60)} h` : `hace ${Math.round(minutos / 1440)} días`;
  caja.classList.add(minutos <= CONFIG.avisoSinDatosMinutos ? 'ok' : 'aviso');
  $('estadoTexto').textContent = minutos <= CONFIG.avisoSinDatosMinutos
    ? `En línea · última medición ${hace}`
    : `Sin datos nuevos · última medición ${hace}`;
}

function extremos(filas, clave) {
  const numeros = filas.map(f => f[clave]).filter(v => tipoDe(v) === 'ok');
  if (numeros.length === 0) return null;
  return { min: Math.min(...numeros), max: Math.max(...numeros) };
}

function dibujarTarjetas() {
  const contenedor = $('tarjetas');
  const filas = filasDeEstacion(estado.recientes);
  const ultima = filas[filas.length - 1];
  $('horaActual').textContent = ultima ? '· ' + formatoFechaHora.format(fechaDe(ultima)) : '';
  const periodo = filasDelPeriodo();

  contenedor.replaceChildren(...VARIABLES.map(v => {
    const valor = ultima ? ultima[v.clave] : null;
    const tipo = tipoDe(valor);
    const tarjeta = document.createElement('div');
    tarjeta.className = 'tarjeta' + (tipo === 'err' ? ' falla' : tipo === 'nan' ? ' sin-sensor' : '');

    const nombre = document.createElement('div');
    nombre.className = 'nombre';
    nombre.textContent = v.nombre;

    const numero = document.createElement('div');
    numero.className = 'valor';
    if (tipo === 'ok') {
      numero.textContent = valor.toFixed(v.decimales);
      const unidad = document.createElement('small');
      unidad.textContent = v.unidad;
      numero.append(unidad);
    } else {
      numero.textContent = tipo === 'err' ? 'ERR' : 'NaN';
    }

    const pie = document.createElement('div');
    pie.className = 'extremos';
    const e = extremos(periodo, v.clave);
    pie.textContent = tipo === 'err' ? 'Falla: revisar el sensor'
      : tipo === 'nan' ? 'Sensor no instalado'
      : e ? `mín ${e.min.toFixed(v.decimales)} · máx ${e.max.toFixed(v.decimales)}` : '';

    tarjeta.append(nombre, numero, pie);
    return tarjeta;
  }));
}

function coloresDelTema() {
  const estilo = getComputedStyle(document.documentElement);
  return { borde: estilo.getPropertyValue('--borde').trim(), texto: estilo.getPropertyValue('--texto-suave').trim() };
}

// Con muchos meses hay decenas de miles de puntos: se agrupan en tramos de tiempo iguales y
// se dibuja el promedio de cada uno. Un tramo sin datos queda vacio y la linea se corta ahi.
const PUNTOS_MAXIMOS = 600;
function resumir(puntos) {
  if (puntos.length <= PUNTOS_MAXIMOS) return puntos;
  const inicio = puntos[0].x;
  const ancho = (puntos[puntos.length - 1].x - inicio) / PUNTOS_MAXIMOS || 1;
  const tramos = [];
  puntos.forEach(p => {
    const i = Math.min(PUNTOS_MAXIMOS - 1, Math.floor((p.x - inicio) / ancho));
    const t = tramos[i] || (tramos[i] = { suma: 0, cantidad: 0 });
    if (p.y !== null) { t.suma += p.y; t.cantidad++; }
  });
  const resultado = [];
  for (let i = 0; i < PUNTOS_MAXIMOS; i++) {
    const t = tramos[i];
    resultado.push({ x: inicio + (i + 0.5) * ancho, y: t && t.cantidad ? t.suma / t.cantidad : null });
  }
  return resultado;
}

function dibujarGraficos() {
  const filas = filasDelPeriodo();
  const contenedor = $('graficos');
  const [desde, hasta] = periodoElegido();
  $('resumenRango').textContent = `· ${filas.length} mediciones · ${desde} a ${hasta}`;
  const tema = coloresDelTema();

  Object.values(estado.graficos).forEach(g => g.destroy());
  estado.graficos = {};
  contenedor.replaceChildren();

  VARIABLES.forEach(v => {
    const todos = filas.map(f => ({ x: fechaDe(f).getTime(), y: tipoDe(f[v.clave]) === 'ok' ? f[v.clave] : null }));
    const conDato = todos.filter(p => p.y !== null).length;
    const puntos = resumir(todos);
    const fallas = filas.filter(f => tipoDe(f[v.clave]) === 'err').length;

    const caja = document.createElement('div');
    caja.className = 'grafico';
    const titulo = document.createElement('h3');
    titulo.textContent = `${v.nombre} (${v.unidad})`;
    const info = document.createElement('span');
    if (fallas > 0) {
      info.className = 'falla';
      info.textContent = `${fallas} con ERR`;
    } else {
      info.textContent = conDato ? `${conDato} datos` : '';
    }
    titulo.append(info);
    caja.append(titulo);

    if (conDato === 0) {
      const vacio = document.createElement('div');
      vacio.className = 'vacio';
      vacio.textContent = filas.length === 0 ? 'No hay mediciones en este período'
        : fallas > 0 ? 'Todas las mediciones del período dieron ERR'
        : 'Sin datos en este período (sensor no instalado)';
      caja.append(vacio);
      contenedor.append(caja);
      return;
    }

    const lienzo = document.createElement('div');
    lienzo.className = 'lienzo';
    const canvas = document.createElement('canvas');
    canvas.setAttribute('role', 'img');
    canvas.setAttribute('aria-label', `Gráfico de ${v.nombre}`);
    lienzo.append(canvas);
    caja.append(lienzo);
    contenedor.append(caja);

    estado.graficos[v.clave] = new Chart(canvas, {
      type: 'line',
      data: {
        datasets: [{
          label: v.nombre, data: puntos, borderColor: v.color, backgroundColor: v.color + '22',
          borderWidth: 2, pointRadius: puntos.length > 60 ? 0 : 2, pointHoverRadius: 4,
          // Resumido, cada tramo vacio ya es un null que corta la linea. Sin resumir, la linea
          // se corta donde faltan mediciones por mas de HUECO_MAXIMO_MS (estacion apagada).
          fill: true, tension: 0.25, spanGaps: puntos === todos ? HUECO_MAXIMO_MS : false,
        }],
      },
      options: {
        parsing: false, normalized: true, animation: false, maintainAspectRatio: false,
        interaction: { mode: 'nearest', axis: 'x', intersect: false },
        plugins: {
          legend: { display: false },
          tooltip: {
            callbacks: {
              title: items => formatoFechaHora.format(new Date(items[0].parsed.x)),
              label: item => `${item.parsed.y.toFixed(v.decimales)} ${v.unidad}`,
            },
          },
        },
        scales: {
          x: {
            type: 'linear',
            grid: { color: tema.borde },
            ticks: { color: tema.texto, maxTicksLimit: 6, callback: x => formatoCorto.format(new Date(x)) },
          },
          y: { grid: { color: tema.borde }, ticks: { color: tema.texto, maxTicksLimit: 6 } },
        },
      },
    });
  });
}

function celda(valor, decimales) {
  const td = document.createElement('td');
  const tipo = tipoDe(valor);
  td.textContent = tipo === 'ok' && typeof decimales === 'number' ? valor.toFixed(decimales)
    : tipo === 'ok' ? String(valor) : tipo === 'err' ? 'ERR' : 'NaN';
  if (tipo !== 'ok') td.className = tipo;
  return td;
}

function dibujarTabla() {
  const filas = filasDelPeriodo().slice().reverse();  // las mas nuevas arriba
  const paginas = Math.max(1, Math.ceil(filas.length / FILAS_POR_PAGINA));
  estado.pagina = Math.min(estado.pagina, paginas - 1);
  const pagina = filas.slice(estado.pagina * FILAS_POR_PAGINA, (estado.pagina + 1) * FILAS_POR_PAGINA);
  const dec = clave => VARIABLES.find(v => v.clave === clave).decimales;

  $('filas').replaceChildren(...pagina.map(f => {
    const tr = document.createElement('tr');
    const fecha = document.createElement('td');
    fecha.textContent = formatoFechaHora.format(fechaDe(f));
    tr.append(fecha,
      celda(f.temperatura, dec('temperatura')), celda(f.humedad, dec('humedad')),
      celda(f.viento, dec('viento')), celda(f.gas, dec('gas')), celda(f.presion, dec('presion')));
    const direccion = document.createElement('td');
    direccion.textContent = typeof f.direccion === 'string' && f.direccion ? f.direccion : 'NaN';
    if (direccion.textContent === 'ERR') direccion.className = 'err';
    else if (direccion.textContent === 'NaN') direccion.className = 'nan';
    tr.append(direccion, celda(f.luminosidad, dec('luminosidad')));
    return tr;
  }));
  $('pagina').textContent = filas.length ? `Página ${estado.pagina + 1} de ${paginas}` : 'Sin mediciones';
  $('anterior').disabled = estado.pagina === 0;
  $('siguiente').disabled = estado.pagina >= paginas - 1;
}

function dibujarTodo() {
  actualizarSelectorEstaciones();
  dibujarEstado();
  dibujarTarjetas();
  dibujarGraficos();
  dibujarTabla();
}

function mostrarError(error) {
  const aviso = document.createElement('div');
  aviso.className = 'mensaje-error';
  aviso.textContent = 'No se pudieron leer los datos: ' + error.message +
    '. Revisar la URL en dashboard/config.js y que el script esté publicado.';
  $('tarjetas').replaceChildren(aviso);
  $('estado').className = 'estado falla';
  $('estadoTexto').textContent = 'Sin conexión con la planilla';
}

// ------------------------------------------------------------------ carga

async function cargarRecientes() {
  estado.recientes = await consultarTramo();  // sin fechas: los ultimos 7 dias
}

async function cargarHistorial() {
  const [desde, hasta] = periodoElegido();
  if (!desde || !hasta || desde > hasta) throw new Error('fechas invalidas');
  estado.historial = await consultar(desde, hasta);
}

async function recargar(conHistorial) {
  try {
    await Promise.all([cargarRecientes(), conHistorial ? cargarHistorial() : null]);
    dibujarTodo();
  } catch (error) {
    mostrarError(error);
  }
}

function elegirRango(rango) {
  estado.rango = rango;
  estado.pagina = 0;
  document.querySelectorAll('.rangos button').forEach(b => b.classList.toggle('activo', b.dataset.rango === rango));
  $('resumenRango').textContent = '· cargando…';
  recargar(true);
}

function descargarCsv() {
  const filas = filasDelPeriodo();
  const columnas = ['fecha', 'codigo', 'temperatura', 'humedad', 'viento', 'gas', 'direccion', 'presion', 'luminosidad'];
  const texto = [columnas.join(';')].concat(filas.map(f => columnas.map(c => {
    const v = f[c];
    if (c === 'fecha' || c === 'codigo' || c === 'direccion') return v || 'NaN';
    return textoValor(v, 2).replace('.', ',');  // coma decimal, para abrirlo en Excel en castellano
  }).join(';'))).join('\r\n');
  const enlace = document.createElement('a');
  enlace.href = URL.createObjectURL(new Blob(['﻿' + texto], { type: 'text/csv;charset=utf-8' }));
  const [desde, hasta] = periodoElegido();
  enlace.download = `mediciones_${estado.estacion}_${desde}_${hasta}.csv`;
  enlace.click();
  URL.revokeObjectURL(enlace.href);
}

function iniciar() {
  const hoy = diaArgentina(new Date());
  $('hasta').value = hoy;
  $('desde').value = diaArgentina(new Date(Date.now() - 7 * MS_POR_DIA));
  $('desde').min = $('hasta').min = CONFIG.inicioHistorial;

  document.querySelectorAll('.rangos button').forEach(b => b.addEventListener('click', () => elegirRango(b.dataset.rango)));
  $('aplicarFechas').addEventListener('click', () => {
    document.querySelectorAll('.rangos button').forEach(b => b.classList.remove('activo'));
    estado.rango = 'fechas';
    estado.pagina = 0;
    recargar(true);
  });
  $('estacion').addEventListener('change', e => {
    estado.estacion = e.target.value;
    estado.pagina = 0;
    dibujarTodo();
  });
  $('anterior').addEventListener('click', () => { estado.pagina--; dibujarTabla(); });
  $('siguiente').addEventListener('click', () => { estado.pagina++; dibujarTabla(); });
  $('descargar').addEventListener('click', descargarCsv);
  window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', dibujarGraficos);

  // ?rango=7 (o 1, 30, todo) en la direccion abre directo en ese periodo: sirve para compartir.
  const rangoPedido = new URLSearchParams(location.search).get('rango');
  if (['1', '7', '30', 'todo'].includes(rangoPedido)) elegirRango(rangoPedido); else recargar(true);
  // Los periodos relativos (24 h, 7, 30 dias) se vuelven a pedir: entran mediciones nuevas.
  setInterval(() => recargar(estado.rango !== 'todo' && estado.rango !== 'fechas'),
    CONFIG.actualizarCadaSegundos * 1000);
}

iniciar();
