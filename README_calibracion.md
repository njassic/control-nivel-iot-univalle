# Calibración del sensor de presión MPX5010DP con Arduino

Práctica de la materia **Control y Simulación de Procesos**: calibración de un sensor de presión diferencial MPX5010DP y su uso para medir el nivel de un tanque.

## Contenido

1. [Definiciones y conceptos](#1-definiciones-y-conceptos)
2. [Calibración de sensores de presión](#2-calibración-de-sensores-de-presión)
3. [Sensor MPX5010DP](#3-sensor-mpx5010dp)
4. [Parte experimental](#4-parte-experimental)
5. [Estructura del repositorio](#5-estructura-del-repositorio)
6. [Subir el proyecto a GitHub](#6-subir-el-proyecto-a-github)

---

## 1. Definiciones y conceptos

**Presión:** fuerza ejercida por unidad de área, `P = F / A` (unidad SI: pascal, Pa).

| Tipo | Se mide respecto a |
|------|--------------------|
| Absoluta | El vacío perfecto |
| Atmosférica | Es la presión del aire a nivel local (~101,3 kPa al nivel del mar) |
| Relativa (manométrica) | La presión atmosférica: `P_rel = P_abs − P_atm` |
| Diferencial | La diferencia entre dos puntos de presión |

Presión hidrostática de una columna de líquido:

```
P = ρ · g · h        →        h = P / (ρ · g)
```

Para agua (ρ = 1000 kg/m³, g = 9,80665 m/s²): **1 kPa ≈ 10,2 cm de columna de agua**.

## 2. Calibración de sensores de presión

Calibrar es comparar la lectura del sensor con valores de referencia conocidos y ajustar la relación entre ambos. Aquí se usa un ajuste lineal de dos puntos:

1. **Cero (offset):** sin presión aplicada se mide `V0`.
2. **Span (sensibilidad):** con una altura de agua conocida `h` se calcula `P = ρ·g·h` y la sensibilidad `S = (V − V0) / P`.

```
P (kPa) = (Vout − V0) / S
```

## 3. Sensor MPX5010DP

### Características (hoja de datos, Vs = 5 V)

| Parámetro | Valor |
|-----------|-------|
| Rango de presión | 0 a 10 kPa |
| Alimentación (Vs) | 4,75 a 5,25 V |
| Salida | 0,2 V (0 kPa) a 4,7 V (10 kPa) |
| Sensibilidad | 450 mV/kPa |
| Función de transferencia | `Vout = Vs · (0,09 · P + 0,04)` |
| Tipo | Diferencial, con salida analógica amplificada |

### Funcionamiento

Un diafragma de silicio con un puente resistivo (piezorresistivo) se deforma según la diferencia de presión entre sus dos puertos. El circuito integrado del sensor amplifica y compensa esa señal, y entrega un voltaje proporcional a la presión.

### Pines

| Pin | Función |
|-----|---------|
| 1 | Vout (señal) |
| 2 | GND |
| 3 | Vs (+5 V) |
| 4, 5, 6 | Sin conexión (N/C) |

> El pin 1 se identifica por la muesca del encapsulado. Verifícalo en la hoja de datos antes de conectar.

## 4. Parte experimental

### Materiales

- Arduino UNO
- Sensor MPX5010DP
- Manguera de silicona delgada y conectores
- Tanque o recipiente transparente con agua
- Regla o cinta métrica
- Cables y protoboard
- Opcional: multímetro

### Ensamblaje del circuito

| MPX5010DP | Arduino UNO | Color de cable |
|-----------|-------------|----------------|
| Pin 1 (Vout) | A0 | Naranja |
| Pin 2 (GND) | GND | Negro |
| Pin 3 (Vs) | 5V | Rojo |

**Montaje hidráulico:** conecta la manguera al puerto de presión (P1) y sumerge el otro extremo en el tanque. El aire atrapado dentro de la manguera funciona como colchón: al subir el nivel `h`, comprime el aire y el sensor mide esa presión. El puerto restante queda abierto a la atmósfera.

### Procedimiento de calibración

1. Carga `mpx5010dp_calibracion/mpx5010dp_calibracion.ino` en el Arduino.
2. Abre el Monitor Serial a **9600 baudios** con "Nueva línea" activado.
3. **Cero:** con la manguera fuera del agua (o a nivel 0), envía `z`.
4. **Span:** sumerge la manguera una altura conocida, por ejemplo 10 cm, y envía `k 10`.
5. Verifica con otras alturas (5, 10, 15, 20 cm) y compara con la regla.
6. Registra los datos para el informe.

### Comandos del Monitor Serial

| Comando | Acción |
|---------|--------|
| `z` | Tara: fija el voltaje actual como cero |
| `k <cm>` | Calibra sensibilidad con una altura conocida |
| `r` | Restaura valores de la hoja de datos |
| `p` | Muestra parámetros de calibración |
| `s` | Pausa o reanuda el envío de datos |
| `h` | Ayuda |

### Formato de salida

```
tiempo_ms,adc,voltaje_V,presion_kPa,nivel_cm
```

Puedes copiar los datos del Monitor Serial a un archivo `.csv` dentro de `datos/` y graficarlos en Excel, Python o MATLAB.

### Notas y limitaciones

- Mantén el sensor y la manguera sin burbujas ni dobleces.
- Al comprimirse el aire dentro de la manguera, la lectura se aparta un poco de la ideal. Con alturas pequeñas el error es bajo. Compruébalo en tus resultados.
- El MPX5010DP soporta máximo ~10 kPa (≈ 100 cm de agua). No lo sobrepases.
- Para más precisión, mide el voltaje real de 5 V con un multímetro y ajusta `VREF` en el código.

## 5. Estructura del repositorio

```
mpx5010dp-calibracion/
├── mpx5010dp_calibracion/
│   └── mpx5010dp_calibracion.ino
├── datos/
│   └── .gitkeep
├── .gitignore
└── README.md
```

## 6. Subir el proyecto a GitHub

### Opción A: desde la terminal (Git)

1. Crea un repositorio vacío en GitHub (botón **New**), por ejemplo `mpx5010dp-calibracion`, sin README ni .gitignore.
2. En la carpeta del proyecto ejecuta:

```bash
git init
git add .
git commit -m "Calibración sensor MPX5010DP con Arduino"
git branch -M main
git remote add origin https://github.com/TU_USUARIO/mpx5010dp-calibracion.git
git push -u origin main
```

Si es la primera vez que usas Git, configura antes tu identidad:

```bash
git config --global user.name "Tu Nombre"
git config --global user.email "tu@correo.com"
```

> GitHub no acepta contraseña en `git push`. Usa un *Personal Access Token* o GitHub CLI (`gh auth login`).

### Opción B: sin instalar nada

1. Crea el repositorio en GitHub.
2. Pulsa **Add file → Upload files**.
3. Arrastra la carpeta del proyecto y haz clic en **Commit changes**.

## Autor

Nombre del estudiante: _______________
Materia: Control y Simulación de Procesos
