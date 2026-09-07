/*
  PRUEBA DE BANCO - LM35Z en GPIO34  (version: forma de la señal)

  Un promedio esconde la forma de lo que esta pasando. Este sketch imprime 40 lecturas
  CONSECUTIVAS en crudo, para poder ver si la señal:
    - sube y baja de forma repetitiva  -> el sensor esta oscilando
    - salta entre bloques al azar      -> contacto intermitente
    - se queda quieta                  -> anda bien

  Referencia importante: 142 mV es el PISO del ADC en escala 0 db (la cuenta cero
  traducida a milivoltios). Un valor de 142 significa "cero", no 14.2 grados.

  Monitor serie a 115200.
*/

const int PIN_LM35 = 34;
const int MUESTRAS = 40;

int lecturas[MUESTRAS];

void setup() {
  Serial.begin(115200);
  delay(500);
  analogSetPinAttenuation(PIN_LM35, ADC_0db);

  Serial.println();
  Serial.println("=== Forma de la señal en GPIO34 ===");
  Serial.println("142 = piso del ADC (cero). 250 = 25 C.");
  Serial.println();
}

void loop() {
  // Primero se capturan todas las muestras seguidas, sin imprimir nada en el medio:
  // imprimir por serie tarda milisegundos y deformaria lo que queremos observar.
  unsigned long inicio = micros();
  for (int i = 0; i < MUESTRAS; i++) {
    lecturas[i] = analogReadMilliVolts(PIN_LM35);
  }
  unsigned long duracion = micros() - inicio;

  Serial.print(duracion / MUESTRAS);
  Serial.print(" us/muestra | ");
  for (int i = 0; i < MUESTRAS; i++) {
    Serial.print(lecturas[i]);
    Serial.print(" ");
  }
  Serial.println();

  delay(1000);
}
