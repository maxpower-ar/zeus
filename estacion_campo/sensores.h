// Lectura de los sensores del nodo de campo.
//
// Regla del proyecto: toda funcion de lectura informa ademas si el dato es VALIDO.
// Un sensor desconectado tiene que producir un dato invalido, nunca un cero silencioso.

#ifndef SENSORES_H
#define SENSORES_H

#include <Arduino.h>

// Una magnitud leida, con su bandera de validez.
struct Lectura {
  float valor;
  bool valido;
};

struct LecturaDht {
  Lectura temperatura;
  Lectura humedad;
};

void iniciarSensores();

// Temperatura y humedad del DHT22.
LecturaDht leerDht();

// Temperatura de un LM35 conectado a un pin de ADC1.
// 'offset' es la correccion medida contra el UT890C para ese sensor.
Lectura leerLm35(int pin, float offset);

#endif
