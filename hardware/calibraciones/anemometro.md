# Calibración del anemómetro

Instrumento: rotor de 3 cazoletas sobre rulemanes 10×22×6, disco ranurado y encoder
óptico FC-03 en GPIO18. Sketch: `pruebas/anemometro/`.

Instrumento de referencia: multímetro UT890C para las tensiones, velocímetro del auto para
la velocidad de viento.

---

## 1. Ranuras del disco — RESUELTO

**30 ranuras**, confirmado el 2026-10-14 contra el modelo OpenSCAD que genera la pieza
(`enc_ranuras = 30`). La contradicción anterior con `enc_ranuras = 20` correspondía al
disco viejo de 36 mm, que quedó obsoleto: con 36 mm no entraban 30 ranuras.

| Parámetro | Valor |
|---|---|
| Diámetro | 69 mm |
| Espesor | 2 mm |
| Ranuras | **30** |
| Banda radial de las ranuras | r = 23.5 a 33.5 mm |
| Ancho de ranura | 2.4 mm (interior) → 3.5 mm (exterior) |
| Borde macizo exterior | 1.0 mm |
| Buje | 26 mm, agujero en D de 16 mm, prisionero M3 |

Ciclo de trabajo verificado: **48.8 % a 49.9 %** en toda la banda radial (ideal `πr/30`).

Fuente: `hardware/modelos/encoder_disco.scad` (copia de la pieza en el repositorio).

### Verificación física pendiente

Aunque el número sale del modelo que genera la pieza, conviene confirmarlo una vez sobre
el disco impreso: sketch `pruebas/encoder_optico`, marcar la rueda, teclear `r`, dar una
vuelta lenta con el dedo, leer el total. Dos minutos.

| Fecha | Ranuras contadas | Quién |
|---|---|---|
| | | |

### Montaje del sensor

Haz del FC-03 a **r ≈ 28.5 mm** (centro de la banda). Hay 10 mm de tolerancia radial, pero
no acercarse al borde: pasado r = 33.5 solo queda 1 mm de disco macizo.

**Control de alabeo:** ese borde de 1 mm es el anillo que mantiene unidos los 30 dientes, y
en PLA de 2 mm a 69 mm de diámetro es delicado. Apoyar una regla de canto sobre el disco
recién impreso. Si se comba, subir `enc_margen` a 1.5 o 2 mm no cuesta nada.

| Fecha | Alabeo medido | Radio del haz | Roza? |
|---|---|---|---|
| | | | |

---

## 2. Geometría del rotor — PENDIENTE

| Dato | Valor | Cómo se mide |
|---|---|---|
| Radio del rotor | *(pendiente)* | Del centro del eje al centro de una cazoleta, con regla |
| Diámetro de cazoleta | *(pendiente, ~50 mm según el STL)* | |
| Cantidad de cazoletas | 3 | |
| Rodamientos | 2 × 10×22×6 | |

No hacen falta para medir —la calibración los absorbe a los dos— pero sirven para
verificar que el resultado tenga sentido físico.

---

## 3. Constante de calibración `metrosPorPulso` — PENDIENTE

Es la única constante que convierte pulsos en velocidad:

```
velocidad [m/s] = metrosPorPulso × pulsos_por_segundo
velocidad [km/h] = velocidad [m/s] × 3.6        (exacto, no es calibración)
```

Equivale a `factor_anemómetro × 2π × radio / ranuras`. Se mide el producto completo de
una sola vez, porque el factor y el radio no se pueden separar con precisión.

**Valor provisional en el código: `0.0471`** — supone radio 0.090 m y factor 2.5, que son
cifras típicas pero **no son las de este rotor**. No sirve como medición.

### Método del auto

Anemómetro firme fuera de la ventanilla, lo más lejos posible de la carrocería (el aire
pegado al auto va más lento que el auto). Camino recto, día sin viento, velocidad
constante. Esperar ~5 s a que los pulsos/s se estabilicen y teclear `c 40` en el monitor
serie, siendo 40 los km/h del velocímetro.

**Cada velocidad se hace de ida y de vuelta y se promedia.** Si hay aunque sea poco
viento, de ida se suma y de vuelta se resta; promediando se cancela.

Maneja otra persona.

| Fecha | Referencia (km/h) | Sentido | pulsos/s | `metrosPorPulso` |
|---|---|---|---|---|
| | 20 | ida | | |
| | 20 | vuelta | | |
| | 30 | ida | | |
| | 30 | vuelta | | |
| | 40 | ida | | |
| | 40 | vuelta | | |
| | 50 | ida | | |
| | 50 | vuelta | | |

**Valor adoptado:** *(pendiente)*

**Criterio:** los valores de las distintas velocidades tienen que dar **parecidos entre
sí**. Si dan parecidos, la relación es lineal como se espera y se promedian. Si dan muy
distintos, hay algo mal en el montaje y no se puede seguir: anotarlo abajo.

---

## 4. Velocidad de arranque — PENDIENTE

Por debajo de qué viento el rotor no arranca. Es el límite conocido del instrumento y hay
que documentarlo: por debajo de ese valor la estación informa calma aunque haya brisa.

| Fecha | Velocidad de arranque (km/h) | Cómo se determinó |
|---|---|---|
| | | |

---

## 5. Comparación de rotores y fricción — PENDIENTE

Tiempo de giro libre tras un empujón: más tiempo = menos fricción = detecta brisa más
suave.

| Montaje | Tiempo de giro libre | Observaciones |
|---|---|---|
| Rotor sobre rulemanes 10×22×6 | | |
| Cooler Jamicon (si se prueba) | | |
| Eje del kit TT (si se prueba) | | |

---

## 6. Verificación contra referencia externa — PENDIENTE

Una vez calibrado, contrastar en un día ventoso contra lo que informa el servicio
meteorológico local. No van a coincidir exactamente —la estación oficial mide a 10 m de
altura y en campo abierto— pero el orden de magnitud y la evolución tienen que seguirse.

| Fecha | Estación propia | Servicio meteorológico | Diferencia |
|---|---|---|---|
| | | | |

---

## Observaciones
