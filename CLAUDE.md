# CLAUDE.md — Estación Meteorológica ESP32

Guía de trabajo para Claude Code en este repositorio. Léela antes de tocar código.

## Qué es este proyecto

Estación meteorológica con **un solo ESP32**, **proyecto de escuela secundaria**. Prioridad
absoluta: que funcione, se entienda y se pueda explicar. La elegancia y la optimización vienen
después, si sobra tiempo.

- **Estación** (`estacion_campo/`, placa **"Delta"**): ESP32 con sensores, a la intemperie.
  Mide cada N minutos, guarda cada medición en su memoria flash interna (LittleFS) y la manda
  por **WiFi** a una API del ITR.
- **Servidor del ITR**: lo hace el equipo de software y **no forma parte de este proyecto**.
  Nosotros garantizamos el formato de línea acordado en `docs/servidor.md`. Para tocar su
  código o cambiar el contrato, se consulta con ellos.
- **Retirado (2026-10-27):** el nodo puente USB **"Beta"** (`estacion_interfaz/`) y el enlace
  ESP-NOW. No escribir código nuevo para ellos.

## Reglas de oro

1. **El formato del dato y de la línea al servidor viven en `libraries/EstacionComun/`, nunca
   duplicados.** Si el struct `Medicion` o el armado de la línea se copian en el firmware y en
   los sketches de `pruebas/`, tarde o temprano divergen. Cambiar el formato de la línea
   rompe al servidor: **se acuerda antes con el equipo de software**.
2. **Una fase por vez.** El plan está en `docs/plan.md`. No adelantar trabajo de fases
   futuras ni implementar lo marcado como OPCIONAL antes de terminar el núcleo.
3. **Ningún valor de calibración se inventa.** Toda constante que traduzca voltaje a magnitud
   física sale de una medición real con el multímetro UT890C, registrada en
   `hardware/calibraciones/`. Si todavía no se midió, se deja el valor provisional marcado
   con `TODO: calibrar` y se dice explícitamente en la respuesta al usuario.
4. **Anotar lo que se probó.** Cada sesión de banco deja una entrada en `docs/bitacora.md`:
   qué se conectó, qué se midió, qué falló. Es la memoria del proyecto.
5. **No afirmar que algo funciona sin haberlo verificado en la placa.** El código que compila
   no es código que funciona. Si no se probó en hardware, decirlo.

## Entorno de desarrollo

**Arduino IDE clásico** (no PlatformIO). El repositorio **es el sketchbook**:

> Arduino IDE → Archivo → Preferencias → Ubicación del sketchbook →
> la raíz de este repositorio (en la máquina actual, `C:\Users\Max\Documents\Max\zeus`)

Con eso el IDE descubre solo los dos sketches, los de `pruebas/` y la librería compartida en
`libraries/`.
Es lo que permite compartir código sin duplicarlo.

- Placa: **ESP32 Dev Module** (DevKit V1, 30 pines, ESP32-WROOM-32).
- Esquema de particiones: el que incluya SPIFFS/LittteFS (para el log del nodo de campo).
- Velocidad del monitor serie: **115200**.

Claude no puede compilar ni subir firmware: **el usuario compila y prueba en su máquina**.
Cuando se entregue código, indicar qué hay que observar en el monitor serie para saber si
anduvo.

## Restricciones de hardware que condicionan todo el diseño

Estas no son preferencias, son limitaciones físicas del ESP32. Violarlas produce bugs que
parecen misteriosos.

- **ADC2 no funciona con WiFi activo** (y la estación usa WiFi siempre). Toda entrada analógica va obligatoriamente a
  ADC1: GPIO **32, 33, 34, 35, 36 (VP), 39 (VN)**. Sin excepción.
- **GPIO 0, 2, 12 y 15 son pines de strapping.** Una señal con pull-up conectada ahí puede
  impedir que la placa arranque. No usarlos para entradas de sensores.
- **GPIO 34, 35, 36 y 39 son solo entrada.** No tienen pull-up interno ni pueden ser salida.
- **El ADC del ESP32 no es lineal** en los extremos del rango. Las lecturas cerca de 0 V y
  cerca de 3.3 V no son confiables; hay que trabajar en la zona central.
- **El conversor de nivel es para señales digitales.** Para señales analógicas que superen
  3.3 V hay que usar un divisor resistivo, no el conversor.

El detalle completo de pines está en `docs/hardware/pinout.md`. **Antes de asignar cualquier
pin nuevo, leer ese archivo** — no reasignar pines de memoria.

## Estructura del repositorio

```
estacion_campo/          Firmware de la estación (placa Delta)
estacion_interfaz/       RETIRADO: nodo puente USB (placa Beta), se conserva como referencia
libraries/EstacionComun/ Código compartido: Medicion y formato de la línea al servidor
pruebas/                 Sketches de prueba de concepto, uno por sensor. NO son firmware
docs/                    Plan, bitácora, envío al servidor, formato de datos, hardware
hardware/                Lista de componentes y calibraciones reales
servidor/                Nuestro servidor: Google Sheets + Apps Script (Codigo.gs es copia de referencia)
tools/                   Scripts de PC
img/                     Fotos de los componentes
```

## Convenciones de código

- **Comentarios y nombres en español**, sin acentos en identificadores
  (`temperaturaDht`, no `temperaturaDHT22Sensor` ni `temperature`).
- Constantes de configuración (pines, intervalos, calibraciones) van en el `config.h` del
  sketch correspondiente. **Ningún número mágico dentro de la lógica.**
- Un archivo `.h/.cpp` por responsabilidad. Cuando un archivo crece mucho, es señal de que
  hace más de una cosa.
- Toda función que lea un sensor devuelve además si la lectura fue **válida**. Un sensor
  desconectado tiene que producir un dato marcado como inválido, no un cero silencioso.
- Sin `delay()` largos en el bucle principal: la estación tiene que seguir midiendo aunque el
  WiFi o el servidor no respondan.
- **Contraseñas WiFi y claves de la API nunca en el repositorio.** Van en `secretos.h`, que
  está en el `.gitignore`.

## Sobre el hardware disponible

Lo que hay y lo que falta comprar está en `hardware/lista-componentes.md`. **Consultarlo antes
de proponer soluciones que requieran componentes.** No asumir que existe un sensor que no está
en esa lista.

## Estado del proyecto

El avance real se lleva en `docs/plan.md` (casilla marcada = fase terminada y verificada en
hardware). Al empezar una sesión, leer ese archivo para saber dónde estamos.
