# Bitácora

Registro de lo que se probó, se midió y falló. Es la memoria del proyecto: dentro de tres meses
nadie va a acordarse de por qué un sensor está en un pin raro o por qué una constante vale lo
que vale.

**Anotar sobre todo lo que salió mal.** Los éxitos se ven en el código; los fracasos, si no se
escriben, se repiten.

Entrada más reciente arriba.

---

## Datos permanentes del proyecto

Completar en la fase 0.

| Dato | Valor |
|---|---|
| MAC del nodo de campo | *(pendiente, fase 0)* |
| MAC del nodo interfaz | *(pendiente, fase 0)* |
| Versión del Arduino IDE | *(pendiente)* |
| Versión del core ESP32 | *(pendiente)* |
| Puerto COM del nodo de campo | *(pendiente)* |
| Puerto COM del nodo interfaz | *(pendiente)* |
| Ubicación de la estación | *(pendiente)* |
| Altitud del lugar | *(pendiente, se necesita para interpretar la presión)* |

---

## Plantilla de entrada

```
## AAAA-MM-DD — Fase N — Título

**Qué se hizo:**

**Qué se midió:** (valores reales, con el instrumento usado)

**Qué falló:** (y cómo se resolvió, o si quedó pendiente)

**Pendiente para la próxima:**
```

---

## 2026-09-05 — Fase inicial — Estructura del proyecto

**Qué se hizo:** se definió la arquitectura de dos nodos, se creó la estructura de carpetas, el
plan de fases y la documentación técnica inicial.

**Decisiones tomadas:**

- **Arduino IDE** con el repositorio como sketchbook, para poder compartir código entre los dos
  sketches sin duplicarlo (`libraries/EstacionComun/`)
- **ESP-NOW** como transporte, detrás de una interfaz que permita cambiarlo por LoRa más
  adelante sin reescribir el protocolo
- **LittleFS** en la flash interna para el log; microSD queda como fase opcional
- **Deep-sleep diferido a fase opcional**, porque reinicia el ESP32 en cada ciclo y eso vuelve
  mucho más difícil depurar todo lo demás. El proyecto funciona igual alimentado
- Los 8 sensores Hall se usan como **veleta de 8 rumbos**, no para el anemómetro
- La velocidad del viento se mide con el **motor DC como generador**, con calibración por tabla
  de puntos medidos (no por una constante inventada)

**Restricciones de hardware identificadas:**

- ADC2 no funciona con la radio encendida: todas las analógicas van a ADC1 (32, 33, 34, 35, 36, 39)
- GPIO 0, 2, 12, 15 son pines de strapping: ningún sensor con pull-up ahí
- El A3144 necesita 4.5 V mínimo, así que va a 5 V y su salida pasa por el conversor de nivel
- El conversor de nivel no sirve para señales analógicas: anemómetro y gas necesitan divisor

**Pendiente para la próxima:** fase 0 — instalar el entorno, compilar en ambas placas y anotar
las dos direcciones MAC en la tabla de arriba.
