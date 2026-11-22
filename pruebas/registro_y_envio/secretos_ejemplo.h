// PLANTILLA. Copiar este archivo en la misma carpeta con el nombre secretos.h y completarlo
// (o copiar el secretos.h de pruebas/envio_sheets, que es el mismo).
// secretos.h esta en el .gitignore: nunca se sube a GitHub. Esta plantilla si se sube,
// por eso no tiene que tener datos reales.

#pragma once

// Red WiFi. Tiene que ser de 2.4 GHz: el ESP32 no se conecta a redes de 5 GHz.
const char* const WIFI_NOMBRE = "nombre-de-la-red";
const char* const WIFI_CLAVE  = "contrasena-de-la-red";

// URL que da Apps Script al implementar la aplicacion web (termina en /exec).
const char* const URL_SCRIPT = "https://script.google.com/macros/s/PEGAR-ACA-EL-ID/exec";

// La clave que muestra generarClave() en el editor de Apps Script.
const char* const CLAVE_API = "pegar-aca-la-clave";
