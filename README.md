# MorseBerry 🫐

Entrenador de código Morse para Raspberry Pi. Permite practicar Morse mediante un pulsador físico conectado a GPIO, con feedback sonoro por I2S/ALSA y visualización en pantalla OLED SSD1306.

---

## Dependencias

El proyecto usa las siguientes librerías externas:

| Librería | Paquete runtime | Paquete desarrollo | Uso |
|---|---|---|---|
| libgpiod | `libgpiod2` | `libgpiod-dev` | Lectura del pulsador GPIO |
| ALSA | `libasound2` | `libasound2-dev` | Salida de audio por I2S |
| pthread | incluida en glibc | incluida en gcc | Hilos concurrentes |
| libm | incluida en glibc | incluida en gcc | Generación de onda senoidal |

---

## Instalación de dependencias

### Opción A — Script automático (recomendado)

```bash
chmod +x install_deps.sh
sudo ./install_deps.sh
```

### Opción B — Manual

```bash
sudo apt-get update

sudo apt-get install -y \
    gcc \
    make \
    libgpiod2 \
    libgpiod-dev \
    libasound2 \
    libasound2-dev
```

> **Nota Raspberry Pi OS Bookworm (Debian 12):** los nombres de paquete anteriores son correctos.  
> **Nota Bullseye (Debian 11) o anterior:** si `libasound2` falla, prueba con `libasound2-dev` directamente, ya que algunas versiones lo incluyen todo en el paquete dev.

---

## Compilación

```bash
make
```

Para limpiar los binarios generados:

```bash
make clean
```

---

## Uso

```bash
./main -g [pin_gpio] -f [frecuencia]
```

| Argumento | Descripción | Ejemplo |
|---|---|---|
| `-g` | Número de pin GPIO donde está conectado el pulsador | `-g 17` |
| `-f` | Frecuencia del tono Morse en Hz (opcional) | `-f 700` |

Ejemplo completo:

```bash
./main -g 17 -f 700
```

### Navegación por el menú

| Entrada | Acción |
|---|---|
| Pulso corto | Bajar una opción |
| Mantener pulsado (>1.5s) | Confirmar opción seleccionada |
| Tecla `1`-`7` | Seleccionar opción directamente |
| `Espacio` / `S` | Bajar una opción |
| `Enter` | Confirmar opción resaltada |
| `Esc` | Volver al menú |

---

## Hardware necesario

- Raspberry Pi (cualquier modelo con GPIO)
- Pulsador conectado al pin GPIO configurado (con resistencia pull-up interna activada por software)
- Pantalla OLED SSD1306 por I2C
- Salida de audio compatible con ALSA (jack 3.5mm o DAC I2S)

---

## Estructura del proyecto

```
.
├── main.c                  # Punto de entrada, menú principal
├── audio.c / .h            # Hilo de audio ALSA
├── gpio_input.c / .h       # Hilo de lectura del pulsador
├── modos.c / .h            # Modos de entrenamiento
├── pantalla_oled.c / .h    # Driver OLED SSD1306
├── traductor_morse.c / .h  # Lógica de traducción Morse
├── globals.c               # Variables globales compartidas
├── config.h                # Constantes y configuración
├── utils.c / .h            # Utilidades (tiempo, terminal)
├── palabras.c / .h         # Banco de palabras para práctica
├── extern/
│   ├── ssd1306.c / .h      # Driver bajo nivel SSD1306
│   └── ssd1306_fonts.c     # Fuentes para la OLED
├── makefile
├── install_deps.sh
└── README.md
```