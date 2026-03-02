#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <gpiod.h>
#include <time.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>

#include "traductor_morse.h"

#define FREQ 100
#define GPIO_PREDET 17

#define TIEMPO_PUNTO 100
#define TIEMPO_RAYA 300
#define TIEMPO_ESPACIO 700
#define DESVIACION 50

#define SIMBOLO_PUNTO 1
#define SIMBOLO_RAYA 2
#define SIMBOLO_ESPACIO_CORTO 3
#define SIMBOLO_ESPACIO_LARGO 4
#define SIMBOLO_DESCONOCIDO 5

int morse_frecuency = FREQ;
int morse_gpio = GPIO_PREDET;

// variables compartidas para hilos
volatile char simbolo_detectado = 0; // 0 = .   1 = -    2 = espacio
pthread_mutex_t mutex_morse = PTHREAD_MUTEX_INITIALIZER;

struct termios oldt;

// --- MENSAJES ----
void mensaje_menu(){
    printf("\nMenu MorseBerry --- \n");
    printf("Opciones: \n");
    printf("  1 -> Deteccion letra a letra\n");
    printf("  2 -> Modo libre\n");
    printf("  3 -> Salir de aplicacion\n");
}
void mensaje_formato_args()
{
    printf("\nFormato: ./main -g [gpio] -f [frecuencia]\n");
}

// ---- TIMEPO ----
// obtener tiempo actual en milisegundos
long long obtener_tiempo_actual()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    long long res = ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);

    return res;
}

// ---- HILO DE GPIO ----
void *funcion_hilo_gpio(void *arg)
{
    struct gpiod_line_request *request = (struct gpiod_line_request *)arg;
    struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);
    struct gpiod_edge_event *event;

    long long tiempo_pulsado = 0;
    long long tiempo_sin_pulsar = obtener_tiempo_actual();
    int pulsado = 0;
    int ret;

    while (1)
    {

        ret = gpiod_line_request_wait_edge_events(request, 100000000);

        if (ret > 0)
        {
            if (gpiod_line_request_read_edge_events(request, buffer, 1) > 0)
            {
                event = gpiod_edge_event_buffer_get_event(buffer, 0);
                int tipo = gpiod_edge_event_get_event_type(event);

                if (tipo == GPIOD_EDGE_EVENT_FALLING_EDGE)
                { // detecta flanco de bajada -> cuando se pulsa
                    tiempo_pulsado = obtener_tiempo_actual();
                    pulsado = 1;
                }
                else if (tipo == GPIOD_EDGE_EVENT_RISING_EDGE)
                { // detecta flanco subida -> cuando se deja de pulsar
                    int tiempo_ahora = obtener_tiempo_actual();
                    long long duracion_pulsacion = tiempo_ahora - tiempo_pulsado;
                    tiempo_sin_pulsar = obtener_tiempo_actual();
                    pulsado = 0;

                    pthread_mutex_lock(&mutex_morse);
                    if (TIEMPO_PUNTO - DESVIACION <= duracion_pulsacion &&
                        duracion_pulsacion <= TIEMPO_PUNTO + DESVIACION)
                    { // es punto
                        simbolo_detectado = SIMBOLO_PUNTO;
                    }
                    else if (TIEMPO_RAYA - DESVIACION <= duracion_pulsacion &&
                             duracion_pulsacion <= TIEMPO_RAYA + DESVIACION)
                    { // es raya
                        simbolo_detectado = SIMBOLO_RAYA;
                    }
                    else
                    {
                        simbolo_detectado = SIMBOLO_DESCONOCIDO;
                    }
                    pthread_mutex_unlock(&mutex_morse);
                }
            }
        }
        else if (ret == 0 && pulsado == 0)
        { // si no hay eventos y el boton no se pulsa se revisaa si hay espacio largo o corto
            int tiempo_ahora = obtener_tiempo_actual();
            if (TIEMPO_RAYA - DESVIACION <= tiempo_ahora - tiempo_sin_pulsar &&
                        tiempo_ahora - tiempo_sin_pulsar <= TIEMPO_RAYA + DESVIACION)
            { // es espacio corto
                pthread_mutex_lock(&mutex_morse);
                simbolo_detectado = SIMBOLO_ESPACIO_CORTO;
                pthread_mutex_unlock(&mutex_morse);
            }
            else if (tiempo_ahora - tiempo_sin_pulsar > TIEMPO_ESPACIO && tiempo_sin_pulsar != 0)
            {
                pthread_mutex_lock(&mutex_morse);
                simbolo_detectado = SIMBOLO_ESPACIO_LARGO;
                tiempo_sin_pulsar = 0;
                pthread_mutex_unlock(&mutex_morse);
            }
        }
    }

    return NULL;
}

// ---- TERMINAL ----
void activar_modo_raw() {
    struct termios newt;

    tcgetattr(STDIN_FILENO, &oldt); 
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

void restaurar_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

//---- MODOS DE APLICACION ---
void modo_letra_a_letra(){
    printf("\nModo Letra a letra\n");
    printf("Para volver al manu pulse ESC\n");
    
    activar_modo_raw();
    int simbolo_desc_encontrado = 0;

    while(1){
        // mirar si se ha pulsado ESC
        char tecla;
        if (read(STDIN_FILENO, &tecla, 1) > 0) {
            if (tecla == 27) {  // ESC
                break;
            }
        }
        
        char lectura = 0;
        pthread_mutex_lock(&mutex_morse);
        if (simbolo_detectado != 0)
        {
            lectura = simbolo_detectado;
            simbolo_detectado = 0; 
        }
        pthread_mutex_unlock(&mutex_morse);
        if (lectura != 0) {
            if (lectura == SIMBOLO_PUNTO) {
                printf(".");
                morse_avanzar('.');
            }
            else if (lectura == SIMBOLO_RAYA){
                printf("-");
                morse_avanzar('-');
            }
            else if (lectura == SIMBOLO_ESPACIO_LARGO){
                // Obtenemos la letra final y el estado se reinicia solo
                char letra_final = morse_obtener_resultado();
                if (letra_final != '?' && !simbolo_desc_encontrado)
                {
                    printf(" -> [%c]\n", letra_final);
                }
                else
                {
                    printf(" -> [Símbolo no reconocido]\n");
                }
                simbolo_desc_encontrado = 0;
            }
            else if (lectura == SIMBOLO_DESCONOCIDO){
                printf("?");
                simbolo_desc_encontrado = 1;
            }
            fflush(stdout);
        }

        usleep(10000);
    }

   restaurar_terminal();

}

void modo_libre(){
    printf("\nModo Libre\n");
    printf("Para volver al manu pulse ESC\n");
    
    activar_modo_raw();
    int simbolo_desc_encontrado = 0;

    
    while(1){
        // mirar si se ha pulsado ESC
        char tecla;
        if (read(STDIN_FILENO, &tecla, 1) > 0) {
            if (tecla == 27) {  // ESC
                break;
            }
        }

        char lectura = 0;
        pthread_mutex_lock(&mutex_morse);
        if (simbolo_detectado != 0)
        {
            lectura = simbolo_detectado;
            simbolo_detectado = 0;
        }
        pthread_mutex_unlock(&mutex_morse);
        if (lectura != 0) {
            if (lectura == SIMBOLO_PUNTO) {
                morse_avanzar('.');
            }
            else if (lectura == SIMBOLO_RAYA){
                morse_avanzar('-');
            }
            else if (lectura == SIMBOLO_ESPACIO_CORTO){
                // Obtenemos la letra final y el estado se reinicia solo
                char letra_final = morse_obtener_resultado();
                if (letra_final != '?' && !simbolo_desc_encontrado)
                {
                    printf("%c", tolower(letra_final));
                }
                else
                {
                    printf("?");
                }
                simbolo_desc_encontrado = 0;
            }
            else if (lectura == SIMBOLO_ESPACIO_LARGO){
                printf(" ");
                simbolo_desc_encontrado = 0;
            }
            else if (lectura == SIMBOLO_DESCONOCIDO){
                printf("?");
                simbolo_desc_encontrado = 1;
            }
            fflush(stdout);
        }

        usleep(10000);
    }

   restaurar_terminal();
}

/*
    Compilacion:
        gcc main.c -o main -lgpiod -lpthread
*/
int main(int argc, char **argv)
{

    if (argc < 1)
    {
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
    while ((opt = (getopt(argc, argv, "g:f:"))) != -1)
    {
        switch (opt)
        {
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

    struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip)
    {
        perror("Error al abrir chip");
        return EXIT_FAILURE;
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    // se especifica que queremos ambos flancos a detectar
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP); // Resistencia intern
    gpiod_line_settings_set_debounce_period_us(settings, 30000);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = (unsigned int)morse_gpio;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    struct gpiod_line_request *request = gpiod_chip_request_lines(chip, NULL, line_cfg);
    if (!request)
    {
        perror("Error solicitud linea");
        goto close_chip;
    }

    // --- LANZAMIENTO DEL HILO DE INTERRUPCIONES ---
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, funcion_hilo_gpio, (void *)request) != 0)
    {
        perror("Error al crear hilo");
        return 1;
    }

    printf("--- ENTRNADOR MORSE INICIADO (GPIO: %d) ---\n", morse_gpio);
   
    /*
           BUCLE PRINCIPAL INFINITO
       */
    int opcion_menu = 0;
    
    while (1)
    {

        mensaje_menu();
        printf("\n\nOpcion elegida: ");
        while(opcion_menu == 0){
            scanf(" %c", &opcion_menu);
        } 

        if(opcion_menu == '1'){
            modo_letra_a_letra();
            opcion_menu = 0;
        }
        else if(opcion_menu == '2'){
            modo_libre();
            opcion_menu = 0;
        }
        else if(opcion_menu == '3'){
            printf("Saliendo de la aplicacion\n");
            break;
        }
    
    }
    /*
    while (1)
    {
        char lectura = 0;

        mensaje_menu();
        scanf(" %c"&opcion_menu); 
        // leer si gpio tiene algo


        if(opcion_menu == '1'){
            printf("\nModo Letra a letra\n");

            pthread_mutex_lock(&mutex_morse);
            if (simbolo_detectado != 0)
            {
                lectura = simbolo_detectado;
                simbolo_detectado = 0; // Consumimos el símbolo
            }
            pthread_mutex_unlock(&mutex_morse);

            if (lectura != 0)
            {
                if (lectura == SIMBOLO_PUNTO)
                {
                    printf(".");
                    morse_avanzar('.');
                }
                else if (lectura == SIMBOLO_RAYA)
                {
                    printf("-");
                    morse_avanzar('-');
                }
                else if (lectura == SIMBOLO_ESPACIO)
                {
                    // Obtenemos la letra final y el estado se reinicia solo
                    char letra_final = morse_obtener_resultado();
                    if (letra_final != '?' && !simbolo_desc_encontrado)
                    {
                        printf(" -> [%c]\n", letra_final);
                    }
                    else
                    {
                        printf(" -> [Símbolo no reconocido]\n");
                    }
                    simbolo_desc_encontrado = 0;
                }
                else if (lectura == SIMBOLO_DESCONOCIDO)
                {
                    printf("?");
                    simbolo_desc_encontrado = 1;
                }
                fflush(stdout);
            }
        }

        // El main puede hacer otras cosas aquí sin bloquear la detección de Morse
        usleep(10000);
    }
    */
    /*
        LIMPIEZA
    */
    gpiod_line_request_release(request);
close_chip:
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);
    return 0;
}
