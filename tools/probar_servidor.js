// Prueba en la PC la parte de servidor/Codigo.gs que no necesita Google: la expresion regular
// de la linea y la conversion de cada dato (numero, NaN, ERR). Repetirla cada vez que se
// toque el formato.
//
//   node tools/probar_servidor.js
//
// Carga Codigo.gs tal cual (sin copiarlo), asi se prueba el codigo verdadero.

const fs = require('fs');
const path = require('path');
const vm = require('vm');

const archivo = path.join(__dirname, '..', 'servidor', 'Codigo.gs');
const codigo = fs.readFileSync(archivo, 'utf8') +
  '\n;this.__prueba = { PATRON_LINEA, convertirNumero, valorJson };';
const contexto = {};
vm.runInNewContext(codigo, contexto);
const { PATRON_LINEA, convertirNumero, valorJson } = contexto.__prueba;

let fallas = 0;
function verificar(descripcion, obtenido, esperado) {
  const ok = JSON.stringify(obtenido) === JSON.stringify(esperado);
  if (!ok) fallas++;
  console.log((ok ? 'ok    ' : 'FALLA ') + descripcion +
    (ok ? '' : '\n       esperado ' + JSON.stringify(esperado) +
               '\n       obtenido ' + JSON.stringify(obtenido)));
}

// Lo que hace guardarLinea con los grupos de la expresion regular (sin la parte de Google).
function procesar(linea) {
  const m = PATRON_LINEA.exec(linea);
  if (!m) return 'formato invalido';
  const numeros = [m[7], m[8], m[9], m[10]].map(convertirNumero);
  const direccion = (m[11] === 'NaN') ? '' : m[11];
  const presionYLuz = [m[12], m[13]].map(convertirNumero);
  if (numeros.concat(presionYLuz).some(n => n === undefined)) return 'valor no numerico';
  return { codigo: m[6], celdas: numeros.concat([direccion], presionYLuz) };
}

console.log('--- lineas que se ACEPTAN ---');
verificar('ejemplo de docs/servidor.md',
  procesar('[26/10/2026;09:56;EM_ITR_001;26,1;24;12,4;NaN;NaN;1013,2;NaN;EOF]'),
  { codigo: 'EM_ITR_001', celdas: [26.1, 24, 12.4, '', '', 1013.2, ''] });
verificar('DHT22 con falla: ERR en temperatura y humedad',
  procesar('[22/11/2026;10:05;EM_ITR_001;ERR;ERR;3,2;NaN;NaN;NaN;NaN;EOF]'),
  { codigo: 'EM_ITR_001', celdas: ['ERR', 'ERR', 3.2, '', '', '', ''] });
verificar('BMP180 con falla: ERR en presion',
  procesar('[22/11/2026;10:10;EM_ITR_001;22,4;48;0,0;NaN;NaN;ERR;NaN;EOF]'),
  { codigo: 'EM_ITR_001', celdas: [22.4, 48, 0, '', '', 'ERR', ''] });
verificar('temperatura bajo cero y punto decimal',
  procesar('[01/07/2026;06:00;EM_ITR_001;-1.5;90;0;NaN;NaN;1020.3;NaN;EOF]'),
  { codigo: 'EM_ITR_001', celdas: [-1.5, 90, 0, '', '', 1020.3, ''] });
verificar('direccion con rumbo y direccion con ERR',
  [procesar('[01/07/2026;06:05;EM_ITR_001;5;90;2;NaN;NE;NaN;NaN;EOF]').celdas[4],
   procesar('[01/07/2026;06:10;EM_ITR_001;5;90;2;NaN;ERR;NaN;NaN;EOF]').celdas[4]],
  ['NE', 'ERR']);
verificar('-1 es un dato real (-1 C): por eso nunca se usa como marca de falla',
  procesar('[26/10/2026;09:56;EM_ITR_001;-1;24;12,4;NaN;NaN;1013,2;NaN;EOF]').celdas[0], -1);

console.log('--- lineas que se RECHAZAN ---');
verificar('hora con ; en vez de :',
  procesar('[26/10/2026;09;56;EM_ITR_001;26,1;24;12,4;NaN;NaN;1013,2;NaN;EOF]'), 'formato invalido');
verificar('texto cualquiera en un dato',
  procesar('[26/10/2026;09:56;EM_ITR_001;hola;24;12,4;NaN;NaN;1013,2;NaN;EOF]'), 'formato invalido');
verificar('err en minuscula',
  procesar('[26/10/2026;09:56;EM_ITR_001;err;24;12,4;NaN;NaN;1013,2;NaN;EOF]'), 'formato invalido');
verificar('dato vacio',
  procesar('[26/10/2026;09:56;EM_ITR_001;;24;12,4;NaN;NaN;1013,2;NaN;EOF]'), 'formato invalido');
verificar('falta un campo (formato viejo, sin presion ni luminosidad)',
  procesar('[26/10/2026;09:56;EM_ITR_001;26,1;24;12,4;NaN;NaN;EOF]'), 'formato invalido');
verificar('falta EOF',
  procesar('[26/10/2026;09:56;EM_ITR_001;26,1;24;12,4;NaN;NaN;1013,2;NaN]'), 'formato invalido');

console.log('--- JSON de doGet ---');
verificar('celda vacia -> null, ERR -> "ERR", numero -> numero',
  ['', 'ERR', 22.4, 0].map(valorJson), [null, 'ERR', 22.4, 0]);

console.log(fallas === 0 ? '\nTODO BIEN' : '\n' + fallas + ' PRUEBA(S) FALLARON');
process.exit(fallas === 0 ? 0 : 1);
