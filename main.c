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
#include "pantalla_oled.h"
// CONFIGURACIONES POR DEFECTO
#define FREQ 100
#define GPIO_PREDET 17

// TIEMPOS POR DEFECTO
#define TIEMPO_PUNTO 100
#define TIEMPO_RAYA 300
#define TIEMPO_ESPACIO 700
#define DESVIACION 50
#define TIEMPO_MANTENER 1500

// SIMBOLOS
#define SIMBOLO_PUNTO 1
#define SIMBOLO_RAYA 2
#define SIMBOLO_ESPACIO_CORTO 3 // fin de letra
#define SIMBOLO_ESPACIO_LARGO 4 // fin palabra
#define SIMBOLO_DESCONOCIDO 5
#define SIMBOLO_MANTENER_PULSADO 6

int morse_frecuency = FREQ;
int morse_gpio = GPIO_PREDET;
int continuar_ejecucion_hilo = 1; // para indicar al hilo cuando se cierra aplicacion

// variables compartidas para hilo de GPIO
volatile char simbolo_detectado = 0; // 0 = .   1 = -    2 = espacio
pthread_mutex_t mutex_morse = PTHREAD_MUTEX_INITIALIZER;

// restaurar la terminal al salir
struct termios oldt;

// --- MENSAJES ----
void dibujar_menu_interfaz(int opcion_resaltada)
{
    printf("\033[2J\033[H");
    oled_limpiar();

    // 2. DIBUJAR CABECERA
    printf("--- MorseBerry ---\n\n");
    oled_posicionar_cursor(10, 0); // X=10, Y=0 (Página 0)
    oled_imprimir("--- MORSEBERRY ---");

    // 3. DIBUJAR OPCIONES CON RESALTE
    const char *opciones[] = {
        "1. Letra a letra",
        "2. Modo libre",
        "3. Prueba letras",
        "4. Salir"};

    char buffer[20];
    for (int i = 0; i < 4; i++)
    {
        oled_posicionar_cursor(0, 2 + i);
        if ((i + 1) == opcion_resaltada)
        {
            printf(" > %s < \n", opciones[i]); // Resaltado para SSH
            sprintf(buffer, "> OPCION %d", i + 1);
        }
        else
        {
            printf("   %s   \n", opciones[i]);
            sprintf(buffer, "  OPCION %d", i + 1);
        }
        oled_imprimir(buffer);
    }

    printf("\n(Pulso corto: Bajar | Mantener pulsado: OK)\n");
    fflush(stdout);
    // [I2C OLED]: oled_display_update(); // Enviar buffer a la pantalla
}

void mensaje_formato_args()
{
    printf("Formato: ./main -g [gpio] -f [frecuencia]\n");
    printf("Por ahora solo disponible opcion -g\n");
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
    int espacio_corto_emitido = 0;
    int ret;

    while (continuar_ejecucion_hilo)
    {
        // espera 100ms viendo si se produce evento se subida o bajada
        ret = gpiod_line_request_wait_edge_events(request, 100000000);

        if (ret > 0)
        {
            if (gpiod_line_request_read_edge_events(request, buffer, 1) > 0)
            {
                // sacamos el tipo del evento
                event = gpiod_edge_event_buffer_get_event(buffer, 0);
                int tipo = gpiod_edge_event_get_event_type(event);

                // se actua segun el evento sea de bajada o subida
                if (tipo == GPIOD_EDGE_EVENT_FALLING_EDGE)
                { // detecta flanco de bajada -> cuando se pulsa
                    tiempo_pulsado = obtener_tiempo_actual();
                    pulsado = 1;
                    espacio_corto_emitido = 0;
                }
                else if (tipo == GPIOD_EDGE_EVENT_RISING_EDGE)
                {
                    long long tiempo_ahora = obtener_tiempo_actual();
                    long long duracion_pulsacion = tiempo_ahora - tiempo_pulsado;
                    tiempo_sin_pulsar = obtener_tiempo_actual();

                    // SOLUCIÓN: Solo procesar si realmente estábamos en estado "pulsado".
                    // Esto evita que se sobrescriba la señal de salida.
                    if (pulsado == 1)
                    {
                        pulsado = 0;
                        pthread_mutex_lock(&mutex_morse);
                        if (TIEMPO_PUNTO - DESVIACION <= duracion_pulsacion &&
                            duracion_pulsacion <= TIEMPO_PUNTO + DESVIACION)
                        {
                            simbolo_detectado = SIMBOLO_PUNTO;
                        }
                        else if (TIEMPO_RAYA - DESVIACION <= duracion_pulsacion &&
                                 duracion_pulsacion <= TIEMPO_RAYA + DESVIACION)
                        {
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
        }
        else if (ret == 0)
        {
            long long tiempo_ahora = obtener_tiempo_actual();

            if (pulsado == 1)
            {
                // NUEVO: Detectar si se está manteniendo pulsado más de 1.5s
                if (tiempo_ahora - tiempo_pulsado >= TIEMPO_MANTENER)
                {
                    pthread_mutex_lock(&mutex_morse);
                    simbolo_detectado = SIMBOLO_MANTENER_PULSADO;
                    pthread_mutex_unlock(&mutex_morse);

                    // "Engañamos" al estado para no emitir múltiples señales seguidas
                    pulsado = 0;
                    tiempo_sin_pulsar = tiempo_ahora;
                }
            }
            else
            {
                long long tiempo_inactivo = tiempo_ahora - tiempo_sin_pulsar;

                // Evaluamos el espacio largo primero (fin de palabra)
                if (tiempo_inactivo > TIEMPO_ESPACIO && tiempo_sin_pulsar != 0)
                {
                    pthread_mutex_lock(&mutex_morse);
                    simbolo_detectado = SIMBOLO_ESPACIO_LARGO;
                    tiempo_sin_pulsar = 0; // Esto evita que vuelva a entrar
                    pthread_mutex_unlock(&mutex_morse);
                }
                // Evaluamos el espacio corto (fin de letra) asegurándonos de emitirlo solo UNA vez
                else if (tiempo_inactivo >= (TIEMPO_RAYA - DESVIACION) &&
                         tiempo_sin_pulsar != 0 &&
                         espacio_corto_emitido == 0)
                {

                    pthread_mutex_lock(&mutex_morse);
                    simbolo_detectado = SIMBOLO_ESPACIO_CORTO;
                    pthread_mutex_unlock(&mutex_morse);

                    espacio_corto_emitido = 1; // MARCAMOS COMO EMITIDO
                }
            }
        }
    }

    gpiod_edge_event_buffer_free(buffer);
    return NULL;
}

// ---- TERMINAL ----
// se configura para que pueda leer el ESC sin necesidad de esperar al ENTER
void activar_modo_raw()
{
    struct termios newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // lectura no bloqueante
}

void restaurar_terminal()
{
    // Restaurar la configuración original de la terminal
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    // Quitar el modo no bloqueante
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

//---- MODOS DE APLICACION ---
// muestra letra a letra mostrando los puntos y rayas
void modo_letra_a_letra()
{
    printf("\nModo Letra a letra\n");
    printf("Para volver al menu pulse ESC o mantenga pulsado\n");

    activar_modo_raw();
    int simbolo_desc_encontrado = 0;

    while (1)
    {
        // mirar si se ha pulsado ESC
        char tecla;
        if (read(STDIN_FILENO, &tecla, 1) > 0)
        {
            if (tecla == 27)
            { // ESC
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

        if (lectura != 0)
        {
            if (lectura == SIMBOLO_MANTENER_PULSADO)
            {
                // [I2C OLED]: Mostrar mensaje de "Volviendo al menú..."
                break; // Sale del bucle del modo y vuelve al while del main()
            }
            else if (lectura == SIMBOLO_PUNTO)
            {
                printf(".");
                morse_avanzar('.');
            }
            else if (lectura == SIMBOLO_RAYA)
            {
                printf("-");
                morse_avanzar('-');
            }
            else if (lectura == SIMBOLO_ESPACIO_LARGO)
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

        usleep(10000);
    }

    restaurar_terminal();
}

void modo_libre()
{
    printf("\nModo Libre\n");
    printf("Para volver al menu pulse ESC o mantenga pulsado\n");

    activar_modo_raw();
    int simbolo_desc_encontrado = 0;

    while (1)
    {
        // mirar si se ha pulsado ESC
        char tecla;
        if (read(STDIN_FILENO, &tecla, 1) > 0)
        {
            if (tecla == 27)
            { // ESC
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

        if (lectura != 0)
        {
            if (lectura == SIMBOLO_MANTENER_PULSADO)
            {
                // [I2C OLED]: Mostrar mensaje de "Volviendo al menú..."
                break; // Sale del bucle del modo y vuelve al while del main()
            }
            else if (lectura == SIMBOLO_PUNTO)
            {
                morse_avanzar('.');
            }
            else if (lectura == SIMBOLO_RAYA)
            {
                morse_avanzar('-');
            }
            else if (lectura == SIMBOLO_ESPACIO_CORTO)
            {
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
            else if (lectura == SIMBOLO_ESPACIO_LARGO)
            {
                printf(" ");
                simbolo_desc_encontrado = 0;
            }
            else if (lectura == SIMBOLO_DESCONOCIDO)
            {
                printf("?");
                simbolo_desc_encontrado = 1;
            }
            fflush(stdout);
        }

        usleep(10000);
    }

    restaurar_terminal();
}

/* Genera letra aleatorio A-Z*/
char generar_char_random()
{
    return 'A' + rand() % 26;
}

void modo_prueba_letras()
{
    printf("\nModo prueba de letra: escribe correctamente las letras propuestas\n");
    printf("Para volver al menu pulse ESC o mantenga pulsado\n");

    activar_modo_raw();
    int simbolo_desc_encontrado = 0;
    char caracter_aleatorio = generar_char_random();
    printf("Escribe %c : ", caracter_aleatorio);
    fflush(stdout);

    while (1)
    {
        // mirar si se ha pulsado ESC
        char tecla;
        if (read(STDIN_FILENO, &tecla, 1) > 0)
        {
            if (tecla == 27)
            { // ESC
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

        if (lectura != 0)
        {
            if (lectura == SIMBOLO_MANTENER_PULSADO)
            {
                // [I2C OLED]: Mostrar mensaje de "Volviendo al menú..."
                break; // Sale del bucle del modo y vuelve al while del main()
            }
            else if (lectura == SIMBOLO_PUNTO)
            {
                printf(".");
                morse_avanzar('.');
            }
            else if (lectura == SIMBOLO_RAYA)
            {
                printf("-");
                morse_avanzar('-');
            }
            else if (lectura == SIMBOLO_ESPACIO_LARGO)
            {
                // Obtenemos la letra final y el estado se reinicia solo
                char letra_final = morse_obtener_resultado();
                if (letra_final != '?' && !simbolo_desc_encontrado && caracter_aleatorio == letra_final)
                {
                    printf(" -> [ACIERTO] - %c\n", letra_final);
                }
                else if (letra_final == '?' || simbolo_desc_encontrado)
                {
                    printf(" -> [FALLO] - Caracter escrito %c\n", '?');
                }
                else
                {
                    printf(" -> [FALLO] - Caracter escrito %c\n", letra_final);
                }
                simbolo_desc_encontrado = 0;
                caracter_aleatorio = generar_char_random();
                printf("Escribe %c : ", caracter_aleatorio);
            }
            else if (lectura == SIMBOLO_DESCONOCIDO)
            {
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

    mensaje_formato_args();
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
        return 1;
    }

    printf("Iniciando mi driver OLED por hardware...\n");
    oled_iniciar(); // <--- Aquí llamas a tu driver

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

    // limpia el buffer

    int opcion_resaltada = 1;
    int ejecutar_opcion = 0;

    while (continuar_ejecucion_hilo)
    {
        activar_modo_raw(); // Activar teclado no bloqueante
        dibujar_menu_interfaz(opcion_resaltada);
        ejecutar_opcion = 0;

        // Bucle de espera (lee Morse y Teclado a la vez)
        while (ejecutar_opcion == 0 && continuar_ejecucion_hilo)
        {

            // --- 1. LEER TECLADO SSH ---
            char tecla = 0;
            if (read(STDIN_FILENO, &tecla, 1) > 0)
            {
                if (tecla == ' ' || tecla == 's' || tecla == 'S')
                {
                    // Espacio o 's' simula pulso corto (Bajar en el menú)
                    opcion_resaltada = (opcion_resaltada % 4) + 1;
                    dibujar_menu_interfaz(opcion_resaltada);
                }
                else if (tecla == '\n' || tecla == '\r')
                {
                    // ENTER simula pulso largo (Aceptar)
                    ejecutar_opcion = opcion_resaltada;
                }
                else if (tecla == 27)
                {
                    // Tecla ESC para salir directamente
                    ejecutar_opcion = 4;
                }
            }

            // --- 2. LEER MANIPULADOR MORSE ---
            char lectura = 0;
            pthread_mutex_lock(&mutex_morse);
            if (simbolo_detectado != 0)
            {
                lectura = simbolo_detectado;
                simbolo_detectado = 0;
            }
            pthread_mutex_unlock(&mutex_morse);

            if (lectura != 0)
            {
                if (lectura == SIMBOLO_PUNTO || lectura == SIMBOLO_RAYA)
                {
                    // Pulso corto: Avanzar a la siguiente opción cíclicamente
                    opcion_resaltada = (opcion_resaltada % 4) + 1;
                    dibujar_menu_interfaz(opcion_resaltada);
                }
                else if (lectura == SIMBOLO_MANTENER_PULSADO)
                {
                    // Pulso largo: Seleccionar opción actual
                    ejecutar_opcion = opcion_resaltada;
                }
            }

            usleep(10000); // Pausa de 10ms para no saturar la CPU al 100%
        }

        // --- 3. EJECUTAR LA OPCIÓN ---
        if (ejecutar_opcion == 1)
        {
            modo_letra_a_letra();
        }
        else if (ejecutar_opcion == 2)
        {
            modo_libre();
        }
        else if (ejecutar_opcion == 3)
        {
            modo_prueba_letras();
        }
        else if (ejecutar_opcion == 4)
        {
            printf("\033[2J\033[H"); // Limpiar pantalla antes de salir
            printf("Saliendo de MorseBerry...\n");
            continuar_ejecucion_hilo = 0;
        }
        restaurar_terminal();
    }

    /*
        LIMPIEZA
    */
    continuar_ejecucion_hilo = 0;
    pthread_join(thread_id, NULL); // espera terminacion del hilo
    gpiod_line_request_release(request);
    gpiod_chip_close(chip);
    oled_cerrar();
    printf("Aplicacion finalizada correctamente\n");

    return 0;
}
