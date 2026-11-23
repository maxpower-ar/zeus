#pragma once

// Sensores de temperatura, humedad y presion: DHT22 (GPIO4) y BMP180 (I2C, 0x77).
//
// Regla del proyecto: un sensor instalado que no responde NO produce un cero silencioso.
//   - sensor no instalado (config.h)        -> el dato queda "sin sensor" (NaN)
//   - instalado pero la lectura falla        -> el dato queda como FALLA (ERR) y se avisa
//     por el monitor serie con la causa probable
//
// El anemometro tiene su propio modulo (anemometro.h).

#include <Medicion.h>

// Llamar despues de iniciar el bus I2C (Wire.begin).
void iniciarSensores();

// Lee el DHT22 y el BMP180 y completa temperatura, humedad y presion de la medicion.
// 'avisar' = imprimir en el monitor serie las fallas y la lectura del BMP180.
void leerSensores(Medicion& medicion, bool avisar);

// Ultima temperatura leida por el BMP180, solo para comparar con el DHT22 en el monitor
// serie (no se manda al servidor: el formato no tiene ese campo). NAN si no hay.
float temperaturaBmp180();
