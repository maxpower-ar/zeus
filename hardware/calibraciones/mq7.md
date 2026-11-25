# Calibración del MQ-7 (CO)

**Estado: SIN CALIBRAR.** Las constantes de `estacion_campo/config.h` marcadas
`TODO: calibrar` son provisionales. Hasta completar esto, los ppm sirven para ver si el CO
sube o baja, no como valor absoluto.

## Cómo calcula el firmware

1. Lee la tensión del pin `VN` (GPIO39), promedio de 16 lecturas.
2. Deshace el divisor: `V_AO = V_pin × (R_arriba + R_abajo) / R_abajo`.
3. Despeja la resistencia del sensor: `Rs = RL × (VCC − V_AO) / V_AO`, donde RL es la
   resistencia de carga del módulo en paralelo con el divisor.
4. Con la curva del datasheet: `ppm = 99.042 × (Rs / R0) ^ −1.518`.

La curva es del fabricante. Lo que hay que medir nosotros es lo que depende de **este**
módulo: el divisor, VCC, RL y R0.

## Pasos (con el UT890C)

| # | Qué medir | Cómo | Constante en `config.h` |
|---|---|---|---|
| 1 | Las dos resistencias del divisor | Sueltas, antes de soldar | `MQ7_DIVISOR_ARRIBA_KOHM`, `MQ7_DIVISOR_ABAJO_KOHM` |
| 2 | Tensión de alimentación | Entre VCC y GND del módulo, enchufado | `MQ7_TENSION_VCC_V` |
| 3 | RL del módulo | Módulo **sin** alimentar, entre AO y GND | `MQ7_RESISTENCIA_CARGA_KOHM` |
| 4 | R0 | Ver abajo | `MQ7_R0_KOHM` |

**R0:** con el módulo **precalentado** (al menos 24 h encendido la primera vez), al aire
libre y lejos de autos o fuego, apretar `m` en el monitor serie varias veces. El renglón del
MQ-7 dice `R0 para aire limpio: X kOhm` (calculado como `Rs / 27.5`, la relación del
datasheet en aire limpio). Anotar varias lecturas, promediarlas y poner ese número en
`MQ7_R0_KOHM`.

## Registro de mediciones

| Fecha | Qué | Valor | Notas |
|---|---|---|---|
| | | | |

## Limitaciones (para la defensa)

- El datasheet pide alternar el calefactor entre 5 V (60 s) y 1.4 V (90 s) y medir al final
  del ciclo bajo. El módulo lo deja fijo a 5 V: la lectura es **indicativa**, no de
  laboratorio.
- El MQ-7 también reacciona al hidrógeno y depende de la temperatura y la humedad.
- Su rango útil según el datasheet es de 20 a 2000 ppm. Por debajo de 20, el número es una
  extrapolación de la curva.
