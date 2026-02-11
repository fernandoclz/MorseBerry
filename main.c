#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <gpiod.h>

#define FREQ 500
#define GPIO_PREDET 17

int morse_frecuency = FREQ;
int morse_gpio = GPIO_PREDET;


void mensaje_formato_args(){
    printf("\nFormato: ./main -g [gpio] -f [frecuencia]\n");
}

void funcion_pulsador(){
    printf("Boton pulsado\n");
}

/*
    Compilacion: gcc main.c -o main -lgpiod
*/
int main(int argc, char** argv){

    if(argc < 1){
        perror("Error al inicio del programa");
        exit(1);
    }

    /*
        PARSEO COMANDOS
        opciones:
            frecuencia  : -f int
            gpio        : -g int
    */
    int opt;
    while ((opt = (getopt(argc, argv, "g:f:"))) != -1) {
        switch(opt){
            case 'g':
                morse_gpio = atoi(optarg);
            break;
            case 'f':
                morse_frecuency = atoi(optarg);
            break;
            case '?':
                printf("Opcion no disponible\n");
                mensaje_formato_args();
            break;
            default:

            break;
        }
    }

    /*
        CONFIGURACION GPIO
    */

    
    //abrir chip gpio
    struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip) {
        perror("Error al abrir chip");
        return EXIT_FAILURE;
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_FALLING);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = (unsigned int)morse_gpio;
    
    if (gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings) < 0) {
        perror("Error configurando linea");
        goto close_chip;
    }

    struct gpiod_line_request *request = gpiod_chip_request_lines(chip, NULL, line_cfg);
    if (!request) {
        perror("Error en solicitud de linea");
        goto close_chip;
    }

    printf("Programa morse iniciado en GPIO: %d\n", morse_gpio);
 /*
        BUCLE INFINITO DE DETECCION DEL PULSADOR
    */
    struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);
    while (1) {
        // Bloquea hasta que ocurra un evento
        int ret = gpiod_line_request_wait_edge_events(request, -1); // -1 es espera infinita
        if (ret > 0) {
            if (gpiod_line_request_read_edge_events(request, buffer, 1) > 0) {
                funcion_pulsador();
            }
        }
    }


    /*
        LIMPIEZA GPIO
    */
    gpiod_edge_event_buffer_free(buffer);
    gpiod_line_request_release(request);
    close_chip:
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);
    return 0;
}
