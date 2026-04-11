#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <gpiod.h>
#include <pthread.h>

// Cabeceras de nuestros propios módulos
#include "config.h"
#include "utils.h"
#include "audio.h"
#include "gpio_input.h"
#include "modos.h"
#include "pantalla_oled.h"

void mensaje_formato_args() {
    printf("Formato: ./main -g [gpio] -f [frecuencia]\n");
    printf("Por ahora solo disponible opcion -g\n");
}

int main(int argc, char **argv) {
    if (argc < 1) {
        perror("Error al inicio del programa");
        exit(1);
    }

    mensaje_formato_args();

    morse_frecuency = 700;
    morse_gpio = GPIO_PREDET;

    int opt;
    while ((opt = (getopt(argc, argv, "g:f:"))) != -1) {
        switch (opt) {
            case 'g': morse_gpio = atoi(optarg); break;
            case 'f': morse_frecuency = atoi(optarg); break;
            case '?': 
                printf("Opcion no disponible\n");
                mensaje_formato_args();
                break;
        }
    }

    srand(time(NULL));

    // INICIALIZACIÓN GPIO
    struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        perror("Error al abrir chip");
        return EXIT_FAILURE;
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
    gpiod_line_settings_set_debounce_period_us(settings, 5000);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = (unsigned int)morse_gpio;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    struct gpiod_line_request *request = gpiod_chip_request_lines(chip, NULL, line_cfg);
    if (!request) {
        perror("Error solicitud linea");
        return EXIT_FAILURE;
    }

    printf("Iniciando mi driver OLED por hardware...\n");
    oled_inicializar(); 

    // CREACIÓN DE HILOS
    pthread_t thread_id, thread_audio_id;
    if (pthread_create(&thread_id, NULL, funcion_hilo_gpio, (void *)request) != 0) {
        perror("Error al crear hilo GPIO");
        return EXIT_FAILURE;
    }

    if (pthread_create(&thread_audio_id, NULL, hilo_audio_alsa, NULL) != 0) {
        perror("Error al crear hilo de audio");
        return EXIT_FAILURE;
    }

    printf("--- ENTRENADOR MORSE INICIADO (GPIO: %d) ---\n", morse_gpio);

    // BUCLE PRINCIPAL MENÚ
    int opcion_resaltada = 1;
    int ejecutar_opcion = 0;

    while (continuar_ejecucion_hilo) {
        activar_modo_raw();
        dibujar_menu_interfaz(opcion_resaltada);
        ejecutar_opcion = 0;

        while (ejecutar_opcion == 0 && continuar_ejecucion_hilo) {
            // LECTURA DE TECLADO
            char tecla = 0;
            if (read(STDIN_FILENO, &tecla, 1) > 0) {
                if (tecla >= '1' && tecla <= '8') ejecutar_opcion = tecla - '0';
                if (tecla == ' ' || tecla == 's' || tecla == 'S') {
                    opcion_resaltada = (opcion_resaltada % 8) + 1;
                    dibujar_menu_interfaz(opcion_resaltada);
                } else if (tecla == '\n' || tecla == '\r') {
                    ejecutar_opcion = opcion_resaltada;
                } else if (tecla == 27) {
                    ejecutar_opcion = 8;
                }
            }

            // LECTURA DE MORSE
            char lectura = 0;
            pthread_mutex_lock(&mutex_morse);
            if (simbolo_detectado != 0) {
                lectura = simbolo_detectado;
                simbolo_detectado = 0;
            }
            pthread_mutex_unlock(&mutex_morse);

            if (lectura != 0) {
                if (lectura == SIMBOLO_PUNTO || lectura == SIMBOLO_RAYA) {
                    opcion_resaltada = (opcion_resaltada % 8) + 1;
                    dibujar_menu_interfaz(opcion_resaltada);
                } else if (lectura == SIMBOLO_MANTENER_PULSADO) {
                    ejecutar_opcion = opcion_resaltada;
                }
            }
            usleep(10000);
        }

        // EJECUCIÓN DE OPCIONES
        switch (ejecutar_opcion) {
            case 1: modo_letra_a_letra(); break;
            case 2: modo_libre(); break;
            case 3: modo_prueba_letras(); break;
            case 4: modo_prueba_conjunto_letras(2); break;
            case 5: modo_prueba_palabras(); break;
            case 6: modo_configuracion(); break;
            case 7: 
                // Invertimos el estado del sonido
                sonido_activado = !sonido_activado; 
                break;
            case 8:
                printf("\033[2J\033[HSaliendo de MorseBerry...\n");
                continuar_ejecucion_hilo = 0;
                break;
        }
        restaurar_terminal();
    }

    // LIMPIEZA Y CIERRE
    continuar_ejecucion_hilo = 0;
    pthread_join(thread_id, NULL);
    pthread_join(thread_audio_id, NULL); // Buena práctica añadir el cierre del segundo hilo
    gpiod_line_request_release(request);
    gpiod_chip_close(chip);
    oled_cerrar();
    
    printf("Aplicacion finalizada correctamente\n");
    return EXIT_SUCCESS;
}