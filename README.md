# MorseBerry

Entrenador de código Morse para Raspberry Pi. Permite practicar morse mediante un pulsador físico conectado a GPIO o un manipulador, con feedback sonoro por I2S/ALSA y visualización en pantalla OLED SSD1306.

---

## Dependencias

El proyecto usa las siguientes librerías externas:

| Librería | Paquete runtime | Paquete desarrollo | Uso |
|---|---|---|---|
| libgpiod | `libgpiod2` | `libgpiod-dev` | Lectura pulsador GPIO |
| ALSA | `libasound2` | `libasound2-dev` | Salida audio por I2S |
| pthread | incluida en glibc | incluida en gcc | Hilos concurrentes |
| libm | incluida en glibc | incluida en gcc | Generación de onda senoidal |

---

## Instalación de dependencias

**IMPORTANTE**: Se deben activar los buses I2C desde raspi-config --> Interface Options; I2S desde el archivo /boot/firmware/config.txt descomentando dtparam=i2s=on

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

Todos los argumentos son opcionales y tienen opciones por defecto
```bash
./main -g [pin_gpio] -f [frecuencia] -p [manip_punto] -r [manip_raya]
```

| Argumento | Descripción | Ejemplo |
|---|---|---|
| `-g` | Número pin GPIO del pulsador (defecto: 17) | `-g 19` |
| `-f` | Frecuencia de morse en ppm(defecto: 12ppm)  | `-f 15` |
| `-p` | Número pin GPIO del manipulador del punto (defecto: 24)| `-p 23` |
| `-r` | Número pin GPIO del manipulador de la raya (defecto: 23) | `-r 24` |

Ejemplo completo:

```bash
./main -g 19 -f 15 -p 23 -r 24
```

### Navegación por el menú

| Entrada | Acción |
|---|---|
| Pulso corto | Bajar una opción |
| Mantener pulsado (14 veces duración punto) | Confirmar opción seleccionada |
| Teclado numérico | Seleccionar opción directamente |
| `Espacio` / `S` | Bajar una opción |
| `Enter` | Confirmar opción resaltada |
| `Esc` | Volver al menú |

---

## Hardware necesario

- Raspberry Pi (cualquier modelo con GPIO)
- Pulsador conectado al pin GPIO configurado (con resistencia pull-up interna activada por software o condensador)
- (Opcional) Pulsadores conectado a los GPIO configurados para el manipulador (con resistencia pull-up interna activada por software o condensador)
- Pantalla OLED SSD1306 por I2C
- Salida de audio compatible con ALSA (jack 3.5mm o DAC I2S)

---

## Estructura del proyecto

```
.
├── main.c                  # menu principal
├── audio.c / .h            # hilo de audio ALSA
├── gpio_input.c / .h       # hilo de lectura del pulsador
├── modos.c / .h            # modos de entrenamiento
├── pantalla_oled.c / .h    # driver OLED SSD1306
├── traductor_morse.c / .h  # logica de traducción Morse
├── globals.c               # variables globales compartidas
├── config.h                # constantes y configuración
├── utils.c / .h            # utilidades (tiempo, terminal)
├── palabras.c / .h         # banco de palabras
├── extern/
│   ├── ssd1306.c / .h      # driver bajo nivel SSD1306
│   └── ssd1306_fonts.c     # fuentes para la OLED
├── makefile
├── install_deps.sh
└── README.md
```