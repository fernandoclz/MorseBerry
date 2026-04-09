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
#include <string.h>

#include "traductor_morse.h"
#include "pantalla_oled.h"
#include "palabras.h"

#define MAX_PALABRA 50
// CONFIGURACIONES POR DEFECTO
#define FREQ 100
#define GPIO_PREDET 17

#define NUM_MODOS_MENU 7

// TIEMPOS POR DEFECTO -> 12ppm, palabras por minuto
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

// VARIABLES DE TIEMPO
long long tiempo_punto = TIEMPO_PUNTO;
long long tiempo_raya = TIEMPO_RAYA;
long long tiempo_espacio = TIEMPO_ESPACIO;
long long desviacion = DESVIACION;
long long tiempo_mantener = TIEMPO_MANTENER;
/*
    x = 1200/ppm
    tiempo_punto = x
    tiempo_raya = 3x
    tiempo_espacio = 7x
    desviacion = x/2
    tiempo_mantener >= 14x
*/

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

    long long ppm = 1200 / tiempo_punto;
    printf(" Configurado a %lld ppm (duracion punto = %lld ms)\n", ppm, tiempo_punto);

    printf("\n Navegacion con pulsador -> (Pulso corto: Bajar | Mantener pulsado: OK)");
    printf("\n Navegacion con teclado -> (Pulse numero de la opcion)\n\n");

    // 3. DIBUJAR OPCIONES CON RESALTE
    const char *opciones[] = {
        "1. Letra a letra",
        "2. Modo libre",
        "3. Prueba letras",
        "4. Prueba conjunto de letras random",
        "5. Prueba palabras",
        "6. Configuracion",
        "7. Salir"};

    char buffer[20];
    for (int i = 0; i < NUM_MODOS_MENU; i++)
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

    fflush(stdout);
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
                        if (tiempo_punto - desviacion <= duracion_pulsacion &&
                            duracion_pulsacion <= tiempo_punto + desviacion)
                        {
                            simbolo_detectado = SIMBOLO_PUNTO;
                        }
                        else if (tiempo_raya - desviacion <= duracion_pulsacion &&
                                 duracion_pulsacion <= tiempo_raya + desviacion)
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
                if (tiempo_ahora - tiempo_pulsado >= tiempo_mantener)
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
                if (tiempo_inactivo > tiempo_espacio && tiempo_sin_pulsar != 0)
                {
                    pthread_mutex_lock(&mutex_morse);
                    simbolo_detectado = SIMBOLO_ESPACIO_LARGO;
                    tiempo_sin_pulsar = 0; // Esto evita que vuelva a entrar
                    pthread_mutex_unlock(&mutex_morse);
                }
                // Evaluamos el espacio corto (fin de letra) asegurándonos de emitirlo solo UNA vez
                else if (tiempo_inactivo >= (tiempo_raya - desviacion) &&
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

void modo_configuracion()
{
    int continuar_bucle = 1;
    int opcion_resaltada = 1;
    int ejecutar_opcion = 0;

    activar_modo_raw();

    while (continuar_bucle)
    {
        printf("\033[2J\033[H");
        printf("\n -- Configuracion del programa --\n");
        printf("Para volver al menu pulse ESC o mantenga pulsado\n\n");

        printf("%s 1. Establecer PPM\n", opcion_resaltada == 1 ? ">" : " ");
        printf("%s 2. Establecer duracion punto (ms)\n", opcion_resaltada == 2 ? ">" : " ");
        printf("%s 3. Volver\n", opcion_resaltada == 3 ? ">" : " ");

        ejecutar_opcion = 0;

        while (ejecutar_opcion == 0)
        {
            // --- TECLADO ---
            char tecla = 0;
            if (read(STDIN_FILENO, &tecla, 1) > 0)
            {
                if (tecla >= '1' && tecla <= '3')
                {
                    ejecutar_opcion = tecla - '0';
                }
                else if (tecla == ' ' || tecla == 's' || tecla == 'S')
                {
                    opcion_resaltada = (opcion_resaltada % 3) + 1;
                }
                else if (tecla == '\n')
                {
                    ejecutar_opcion = opcion_resaltada;
                }
                else if (tecla == 27)
                {
                    continuar_bucle = 0;
                    opcion_resaltada = 1;
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
                if (lectura == SIMBOLO_PUNTO || lectura == SIMBOLO_RAYA)
                {
                    opcion_resaltada = (opcion_resaltada % 3) + 1;
                    break;
                }
                else if (lectura == SIMBOLO_MANTENER_PULSADO)
                {
                    ejecutar_opcion = opcion_resaltada;
                }
            }

            usleep(10000);
        }

        // ejecutar opcion elegida
        if (ejecutar_opcion == 1)
        { // ppm
            restaurar_terminal();
            int ppm_introducido = 0;
            printf("\n Introduce PPM: ");
            fflush(stdout);

            if (scanf("%d", &ppm_introducido) != 1 || ppm_introducido <= 0)
            {
                printf(" Valor invalido\n");
                sleep(1);
                activar_modo_raw();
                continue;
            }

            long long x = 1200 / ppm_introducido;
            tiempo_punto = x;
            tiempo_raya = 3 * x;
            tiempo_espacio = 7 * x;
            desviacion = x / 2;
            tiempo_mantener = 14 * x;

            printf(" Establecido a %lld ppm", x);
            sleep(1);
            activar_modo_raw();
        }
        else if (ejecutar_opcion == 2)
        { // duracion punto
            restaurar_terminal();
            int punto = 0;
            printf("\n Introduce duracion punto (ms): ");
            fflush(stdout);

            if (scanf("%d", &punto) != 1 || punto <= 0)
            {
                printf(" Valor invalido\n");
                sleep(1);
                activar_modo_raw();
                continue;
            }

            long long x = punto;
            tiempo_punto = x;
            tiempo_raya = 3 * x;
            tiempo_espacio = 7 * x;
            desviacion = x / 2;
            tiempo_mantener = 14 * x;

            printf(" Establecido duracion de punto a %d ms", punto);
            sleep(1);
            activar_modo_raw();
        }
        else if (ejecutar_opcion == 3)
        { // salir

            continuar_bucle = 0;
        }
    }

    restaurar_terminal();
}

/* Genera letra aleatorio A-Z*/
char generar_char_random()
{
    return 'A' + rand() % 26;
}

void generar_palabra_random(int longitud, char *palabra)
{

    for (int i = 0; i < longitud; i++)
    {
        palabra[i] = generar_char_random();
    }
    palabra[longitud] = '\0';
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

    int num_intentos_fallidos = 0;

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
                int fallado = 1;
                if (letra_final != '?' && !simbolo_desc_encontrado && caracter_aleatorio == letra_final)
                {
                    printf(" -> [ACIERTO] - %c\n", letra_final);
                    fallado = 0;
                }
                else if (letra_final == '?' || simbolo_desc_encontrado)
                {
                    printf(" -> [FALLO] - Caracter escrito %c\n", '?');
                    num_intentos_fallidos++;
                }
                else
                {
                    printf(" -> [FALLO] - Caracter escrito %c\n", letra_final);
                    num_intentos_fallidos++;
                }
                simbolo_desc_encontrado = 0;

                if (num_intentos_fallidos <= 3 && fallado)
                {
                    printf("Intentalo de nuevo\n");
                    printf("%d intentos restantes\n", 7 - num_intentos_fallidos);
                }
                else if (num_intentos_fallidos <= 6 && fallado)
                {
                    printf("Intentalo de nuevo\n");
                    printf("%d intentos restantes\n", 7 - num_intentos_fallidos);
                    printf("Ayuda de los simbolo -> ");
                    imprime_letra_como_morse(caracter_aleatorio);
                }
                else if (fallado)
                {
                    num_intentos_fallidos = 0;
                    printf("Fallo, cambio de letra\n");
                    caracter_aleatorio = generar_char_random();
                }
                else
                {
                    num_intentos_fallidos = 0;
                    caracter_aleatorio = generar_char_random();
                }
                printf("\nEscribe %c : ", caracter_aleatorio);
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

void modo_prueba_conjunto_letras(int num)
{
    printf("\nModo prueba de conjunto de letras: escribe correctamente lo propuesto\n");
    printf("Para volver al menu pulse ESC o mantenga pulsado\n");

    activar_modo_raw();
    int simbolo_desc_encontrado = 0;
    char palabra_random[num + 1];
    char palabra_usuario[num + 1];
    int indice = 0;
    generar_palabra_random(num, palabra_random);
    printf("Escribe %s : ", palabra_random);
    fflush(stdout);

    int num_intentos_fallidos = 0;

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
            else if (lectura == SIMBOLO_ESPACIO_CORTO)
            {
                char letra_final = morse_obtener_resultado();

                if (letra_final != '?' && !simbolo_desc_encontrado)
                {
                    if (indice < num)
                        palabra_usuario[indice] = letra_final;
                    indice++;
                    printf("  ->  %c\n", letra_final);
                }
                else
                {
                    if (indice < num)
                        palabra_usuario[indice++] = '?';
                    printf("?");
                }
            }
            else if (lectura == SIMBOLO_ESPACIO_LARGO)
            {
                palabra_usuario[indice] = '\0';
                printf("\n");
                int fallado = 1;

                if (strcmp(palabra_usuario, palabra_random) == 0)
                {
                    printf(" -> [ACIERTO] %s\n", palabra_usuario);
                    fallado = 0;
                    num_intentos_fallidos = 0;
                }
                else
                {
                    printf(" -> [FALLO]\n");
                    printf("         Esperado: %s\n", palabra_random);
                    printf("         Escrito : %s\n", palabra_usuario);
                    num_intentos_fallidos++;
                }

                simbolo_desc_encontrado = 0;
                indice = 0;
                if (fallado)
                {
                    if (num_intentos_fallidos <= 3)
                    {
                        printf("Intentalo de nuevo\n");
                        printf("%d intentos restantes\n", 7 - num_intentos_fallidos);
                    }
                    else if (num_intentos_fallidos <= 6)
                    {
                        printf("Intentalo de nuevo\n");
                        printf("%d intentos restantes\n", 7 - num_intentos_fallidos);
                        printf("Ayuda:\n");

                        // ayuda visual para usuario, muestra patron de cada letra
                        for (int i = 0; i < num; i++)
                        {
                            printf("%c -> ", palabra_random[i]);
                            imprime_letra_como_morse(palabra_random[i]);
                        }
                    }
                    else
                    {
                        printf("Demasiados fallos, nueva palabra\n");
                        num_intentos_fallidos = 0;
                        generar_palabra_random(num, palabra_random);
                    }
                }
                else
                {
                    generar_palabra_random(num, palabra_random);
                }

                printf("\nEscribe %s : ", palabra_random);
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

void modo_prueba_palabras()
{
    printf("\nModo prueba de palabras reales: escribe correctamente lo propuesto\n");
    printf("Para volver al menu pulse ESC o mantenga pulsado\n");

    activar_modo_raw();
    int simbolo_desc_encontrado = 0;
    char palabra_random[MAX_PALABRA];
    char palabra_usuario[MAX_PALABRA];
    int indice = 0;

    strcpy(palabra_random, obtener_palabra_aleatoria());
    printf("Escribe %s : ", palabra_random);
    fflush(stdout);

    int num_intentos_fallidos = 0;

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
            else if (lectura == SIMBOLO_ESPACIO_CORTO)
            {
                char letra_final = morse_obtener_resultado();

                if (letra_final != '?' && !simbolo_desc_encontrado)
                {
                    if (indice < MAX_PALABRA - 1)
                        palabra_usuario[indice] = letra_final;
                    indice++;
                    printf("  ->  %c\n", letra_final);
                }
                else
                {
                    if (indice < MAX_PALABRA - 1)
                        palabra_usuario[indice++] = '?';
                    printf("?");
                }
            }
            else if (lectura == SIMBOLO_ESPACIO_LARGO)
            {
                palabra_usuario[indice] = '\0';
                printf("\n");

                int fallado = 1;

                if (strcmp(palabra_usuario, palabra_random) == 0)
                {
                    printf(" -> [ACIERTO] %s\n", palabra_usuario);
                    fallado = 0;
                    num_intentos_fallidos = 0;
                }
                else
                {
                    printf(" -> [FALLO]\n");
                    printf("         Esperado: %s\n", palabra_random);
                    printf("         Escrito : %s\n", palabra_usuario);
                    num_intentos_fallidos++;
                }

                simbolo_desc_encontrado = 0;
                indice = 0;

                if (fallado)
                {
                    if (num_intentos_fallidos <= 3)
                    {
                        printf("Intentalo de nuevo\n");
                        printf("%d intentos restantes\n", 7 - num_intentos_fallidos);
                    }
                    else if (num_intentos_fallidos <= 6)
                    {
                        printf("Intentalo de nuevo\n");
                        printf("%d intentos restantes\n", 7 - num_intentos_fallidos);
                        printf("Ayuda:\n");

                        // ayuda usuario con patron morse
                        for (int i = 0; palabra_random[i] != '\0'; i++)
                        {
                            printf("%c -> ", palabra_random[i]);
                            imprime_letra_como_morse(palabra_random[i]);
                        }
                    }
                    else
                    {
                        printf("Demasiados fallos, nueva palabra\n");
                        num_intentos_fallidos = 0;
                        strcpy(palabra_random, obtener_palabra_aleatoria());
                    }
                }
                else
                {
                    strcpy(palabra_random, obtener_palabra_aleatoria());
                }

                printf("\nEscribe %s : ", palabra_random);
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
            tiempo punto (tiempo punto) : -f int
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

    srand(time(NULL));

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
    gpiod_line_settings_set_debounce_period_us(settings, 5000);

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
    oled_inicializar(); 

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
                // navegacion con teclado
                if (tecla >= '1' && tecla <= '7')
                {
                    ejecutar_opcion = tecla - '0';
                }

                // navegacion con pulsador
                if (tecla == ' ' || tecla == 's' || tecla == 'S')
                {
                    // Espacio o 's' simula pulso corto (Bajar en el menú)
                    opcion_resaltada = (opcion_resaltada % 7) + 1;
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
                    // Pulso corto: Avanzar a la siguiente opción ciclicamente
                    opcion_resaltada = (opcion_resaltada % 7) + 1;
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

        // --- 3. EJECUTAR LA OPCION ---
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
            modo_prueba_conjunto_letras(2);
        }
        else if (ejecutar_opcion == 5)
        {
            modo_prueba_palabras();
        }
        else if (ejecutar_opcion == 6)
        {
            modo_configuracion();
        }
        else if (ejecutar_opcion == 7)
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
