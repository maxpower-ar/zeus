// Configuracion del panel. Es lo unico que hay que tocar.

const CONFIG = {
  // URL del servidor de Google Sheets (la misma de URL_SCRIPT en secretos.h, la que termina
  // en /exec). El panel solo LEE con ella: para escribir hace falta CLAVE_API, que no esta
  // aca. Si se crea una implementacion nueva del script, cambia la URL: actualizarla aca.
  urlDatos: 'https://script.google.com/macros/s/AKfycbwKLdHaznE3lkWT-dWzqtHapWcWybI08kNKtVwOvldWp_7q5UbRFWUz-loD7PvRKAz9/exec',

  // Estacion que se muestra al abrir (en el selector aparecen todas las que mandaron datos).
  estacionPorDefecto: 'EM_ITR_001',

  // Primer dia con datos: "Todo el historial" consulta desde aca.
  inicioHistorial: '2026-11-01',

  // Cada cuanto se actualiza solo, en segundos. La estacion mide cada 5 minutos.
  actualizarCadaSegundos: 60,

  // Si la ultima medicion es mas vieja que esto, se avisa que la estacion no esta mandando.
  avisoSinDatosMinutos: 20,
};
