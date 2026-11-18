/*
 * Calibración de sensor de presión MPX5010DP
 * Compatible con Arduino UNO y ESP32
 * Materia: Control y Simulación de Procesos
 *
 * ================= MONTAJE =================
 * Arduino UNO:
 *   MPX5010DP pin 1 (Vout) -> A0
 *   MPX5010DP pin 2 (GND)  -> GND
 *   MPX5010DP pin 3 (Vs)   -> 5V
 *
 * ESP32 (IMPORTANTE: el sensor entrega hasta 4.7 V y el ESP32 tolera 3.3 V):
 *   MPX5010DP pin 3 (Vs)   -> VIN / 5V
 *   MPX5010DP pin 2 (GND)  -> GND
 *   MPX5010DP pin 1 (Vout) -> [R1 = 10k] -> nodo -> GPIO34
 *                              nodo -> [R2 = 20k] -> GND
 *   (opcional: condensador de 100 nF entre GPIO34 y GND)
 *
 * Pines 4, 5 y 6 del sensor sin conexión (N/C).
 *
 * ================= FÍSICA =================
 * Nivel por presión: P = rho * g * h
 * Función de transferencia (hoja de datos, Vs = 5 V):
 *   Vout = Vs * (0.09 * P + 0.04)      con P en kPa
 *   => Vout = 0.2 V + 0.45 V/kPa * P   (0 a 10 kPa -> 0.2 a 4.7 V)
 *
 * ================= COMANDOS =================
 * Monitor Serial, enviar con "Nueva línea" (baudios: ver BAUDIOS abajo)
 *   z        Tara: toma el voltaje actual como cero (puertos al aire).
 *   k <cm>   Calibra la sensibilidad con una altura de agua conocida en cm.
 *            Ejemplo: k 10
 *   r        Restaura los valores por defecto de la hoja de datos.
 *   p        Muestra los parámetros de calibración actuales.
 *   s        Inicia o detiene el envío continuo de datos.
 *   h        Muestra esta ayuda.
 *
 * Salida CSV: tiempo_ms,adc,voltaje_V,presion_kPa,nivel_cm
 * (voltaje_V es el voltaje del sensor, ya compensado por el divisor)
 */

#include <Arduino.h>

// ---------------- Configuración según placa ----------------
#if defined(ESP32)
  const uint8_t PIN_SENSOR = 34;      // GPIO34: ADC1, solo entrada (compatible con WiFi)
  const int     ADC_MAX    = 4095;    // ADC de 12 bits
  const float   R1_OHM     = 10000.0; // Resistencia superior del divisor
  const float   R2_OHM     = 20000.0; // Resistencia inferior del divisor
  const float   FACTOR_DIVISOR = R2_OHM / (R1_OHM + R2_OHM);  // = 0.667
  const unsigned long BAUDIOS = 115200;
#else
  const uint8_t PIN_SENSOR = A0;
  const float   VREF       = 5.0;     // Referencia del ADC (V). Mídela con multímetro si puedes.
  const int     ADC_MAX    = 1023;    // ADC de 10 bits
  const unsigned long BAUDIOS = 9600;
#endif

// ---------------- Configuración de medición ----------------
const uint8_t       N_MUESTRAS = 50;   // Muestras para promediar (reduce ruido)
const unsigned long PERIODO_MS = 500;  // Tiempo entre datos enviados

// ---------------- Parámetros físicos del fluido ----------------
const float RHO = 1000.0;     // Densidad del agua (kg/m^3)
const float G   = 9.80665;    // Gravedad (m/s^2)

// ---------------- Parámetros de calibración ----------------
const float V0_DEFECTO   = 0.20;  // V a 0 kPa  (0.04 * 5 V)
const float SENS_DEFECTO = 0.45;  // V/kPa      (0.09 * 5 V)

float v0   = V0_DEFECTO;    // Voltaje en presión cero (offset)
float sens = SENS_DEFECTO;  // Sensibilidad (V/kPa)

bool          enviarDatos = true;
unsigned long ultimoEnvio = 0;

// ---------------- Funciones de medición ----------------

// Promedia varias lecturas. Devuelve el ADC medio y el voltaje del sensor (V).
// En ESP32 usa analogReadMilliVolts (calibrado de fábrica) y deshace el divisor.
void leerSensor(float &adcProm, float &voltSensor) {
  unsigned long sumaAdc = 0;
#if defined(ESP32)
  unsigned long sumaMv = 0;
#endif

  for (uint8_t i = 0; i < N_MUESTRAS; i++) {
    sumaAdc += analogRead(PIN_SENSOR);
#if defined(ESP32)
    sumaMv += analogReadMilliVolts(PIN_SENSOR);
#endif
    delay(2);
  }

  adcProm = (float)sumaAdc / N_MUESTRAS;

#if defined(ESP32)
  float vPin = ((float)sumaMv / N_MUESTRAS) / 1000.0;  // Voltaje en el pin (V)
  voltSensor = vPin / FACTOR_DIVISOR;                  // Voltaje real del sensor (V)
#else
  voltSensor = adcProm * VREF / ADC_MAX;
#endif
}

// Presión manométrica en kPa usando la calibración actual.
float voltajeAPresion(float v) {
  return (v - v0) / sens;
}

// Nivel de líquido en cm a partir de la presión: h = P / (rho * g)
float presionANivelCm(float pKpa) {
  return (pKpa * 1000.0) / (RHO * G) * 100.0;
}

// Presión hidrostática (kPa) de una columna de agua de h cm.
float nivelCmAPresion(float hCm) {
  return RHO * G * (hCm / 100.0) / 1000.0;
}

// ---------------- Salida por serial ----------------

void imprimirAyuda() {
  Serial.println(F("# Comandos: z (tara) | k <cm> (calibrar con altura conocida) | r (reset) | p (parametros) | s (pausar/reanudar) | h (ayuda)"));
}

void imprimirParametros() {
  Serial.print(F("# Parametros -> V0 = "));
  Serial.print(v0, 4);
  Serial.print(F(" V | Sensibilidad = "));
  Serial.print(sens, 4);
  Serial.println(F(" V/kPa"));
}

void imprimirEncabezado() {
  Serial.println(F("tiempo_ms,adc,voltaje_V,presion_kPa,nivel_cm"));
}

void enviarMedicion() {
  float adc, v;
  leerSensor(adc, v);
  float p = voltajeAPresion(v);
  float h = presionANivelCm(p);

  Serial.print(millis());  Serial.print(',');
  Serial.print(adc, 1);    Serial.print(',');
  Serial.print(v, 4);      Serial.print(',');
  Serial.print(p, 4);      Serial.print(',');
  Serial.println(h, 2);
}

// ---------------- Comandos de calibración ----------------

void calibrarCero() {
  float adc, v;
  leerSensor(adc, v);
  v0 = v;
  Serial.print(F("# Tara realizada. Nuevo V0 = "));
  Serial.print(v0, 4);
  Serial.println(F(" V"));
}

void calibrarSensibilidad(float alturaCm) {
  if (alturaCm <= 0) {
    Serial.println(F("# Error: indica una altura mayor que 0 (ej: k 10)"));
    return;
  }
  float pConocida = nivelCmAPresion(alturaCm);
  float adc, v;
  leerSensor(adc, v);
  float nuevaSens = (v - v0) / pConocida;

  if (nuevaSens <= 0) {
    Serial.println(F("# Error: el voltaje no aumento. Revisa mangueras, conexiones y la tara (z)."));
    return;
  }
  sens = nuevaSens;
  Serial.print(F("# Calibrado con "));
  Serial.print(alturaCm, 1);
  Serial.print(F(" cm ("));
  Serial.print(pConocida, 4);
  Serial.print(F(" kPa). Nueva sensibilidad = "));
  Serial.print(sens, 4);
  Serial.println(F(" V/kPa"));
}

void restaurarDefecto() {
  v0   = V0_DEFECTO;
  sens = SENS_DEFECTO;
  Serial.println(F("# Parametros restaurados a valores de la hoja de datos."));
  imprimirParametros();
}

void procesarComando(String linea) {
  linea.trim();
  if (linea.length() == 0) return;

  char  cmd = linea.charAt(0);
  float arg = linea.substring(1).toFloat();

  switch (cmd) {
    case 'z': calibrarCero();             break;
    case 'k': calibrarSensibilidad(arg);  break;
    case 'r': restaurarDefecto();         break;
    case 'p': imprimirParametros();       break;
    case 's':
      enviarDatos = !enviarDatos;
      Serial.println(enviarDatos ? F("# Datos: ACTIVADOS") : F("# Datos: PAUSADOS"));
      break;
    case 'h': imprimirAyuda();            break;
    default:
      Serial.println(F("# Comando no reconocido. Envia h para ayuda."));
  }
}

// ---------------- Arduino ----------------

void setup() {
  Serial.begin(BAUDIOS);

#if defined(ESP32)
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_SENSOR, ADC_11db);  // Rango ~0 a 3.1 V
#else
  pinMode(PIN_SENSOR, INPUT);
#endif

  delay(500);
  Serial.println(F("# Calibracion MPX5010DP - Control y Simulacion de Procesos"));
  imprimirAyuda();
  imprimirParametros();
  imprimirEncabezado();
}

void loop() {
  if (Serial.available()) {
    String linea = Serial.readStringUntil('\n');
    procesarComando(linea);
  }

  if (enviarDatos && millis() - ultimoEnvio >= PERIODO_MS) {
    ultimoEnvio = millis();
    enviarMedicion();
  }
}
