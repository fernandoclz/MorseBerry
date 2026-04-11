#include "modos.h"
#include "config.h"
#include "utils.h"
#include "traductor_morse.h"
#include "pantalla_oled.h"
#include "palabras.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

void dibujar_menu_interfaz(int opcion_resaltada)
{
    printf("\033[2J\033[H");
    oled_limpiar();

    // 1. DIBUJAR CABECERA (Consola SSH)
    printf("--- MorseBerry ---\n\n");
    long long ppm = 1200 / tiempo_punto;
    printf(" Configurado a %lld ppm (duracion punto = %lld ms)\n", ppm, tiempo_punto);
    printf("\n Navegacion con pulsador -> (Pulso corto: Bajar | Mantener pulsado: OK)");
    printf("\n Navegacion con teclado -> (Pulse numero de la opcion)\n\n");

    // 2. DIBUJAR CABECERA (OLED)
    oled_posicionar_cursor(10, 0);
    oled_imprimir("--- MORSEBERRY ---");

    const char *opciones_ssh[] = {
        "1. Letra a letra",
        "2. Modo libre",
        "3. Prueba letras",
        "4. Prueba conjunto de letras random",
        "5. Prueba palabras",
        "6. Configuracion",
        "7. Salir"
    };

    const char *opciones_oled[] = {
        "Letra-Letra",
        "Modo Libre",
        "Pr. Letras",
        "Pr. Conjunto",
        "Pr. Palabras",
        "Configurar",
        "Salir"
    };

    // 3. DIBUJAR OPCIONES EN CONSOLA SSH
    for (int i = 0; i < NUM_MODOS_MENU; i++)
    {
        if ((i + 1) == opcion_resaltada)
            printf(" > %s < \n", opciones_ssh[i]);
        else
            printf("   %s   \n", opciones_ssh[i]);
    }

    // 4. DIBUJAR OPCIONES EN OLED (scroll centrado en la opción resaltada)
    int max_opciones_visibles_oled = 4;

    int indice_inicio_scroll = opcion_resaltada - 1 - (max_opciones_visibles_oled / 2);
    if (indice_inicio_scroll < 0)
        indice_inicio_scroll = 0;
    if (indice_inicio_scroll > NUM_MODOS_MENU - max_opciones_visibles_oled)
        indice_inicio_scroll = NUM_MODOS_MENU - max_opciones_visibles_oled;

    char buffer[21];
    int fila_oled_actual = 2;

    for (int i = indice_inicio_scroll;
         i < (indice_inicio_scroll + max_opciones_visibles_oled) && i < NUM_MODOS_MENU;
         i++)
    {
        oled_posicionar_cursor(0, fila_oled_actual);

        if ((i + 1) == opcion_resaltada)
            snprintf(buffer, sizeof(buffer), "> %s", opciones_oled[i]);
        else
            snprintf(buffer, sizeof(buffer), "  %s", opciones_oled[i]);

        oled_imprimir(buffer);
        fila_oled_actual++;
    }
}

void modo_letra_a_letra()
{
    printf("\nModo Letra a letra\n");
    printf("Para volver al menu pulse ESC o mantenga pulsado\n");

    // OLED:
    // [0] --- MORSEBERRY ---
    // [1]   Letra a letra
    // [2] Morse: .-
    // [3] -> [A]
    oled_limpiar();
    oled_posicionar_cursor(10, 0);
    oled_imprimir("--- MORSEBERRY ---");
    oled_posicionar_cursor(0, 2);
    oled_imprimir("  Letra a letra");
    oled_posicionar_cursor(0, 3);
    oled_imprimir("Morse:");
    oled_posicionar_cursor(0, 4);
    oled_imprimir("-> ");

    activar_modo_raw();
    int simbolo_desc_encontrado = 0;
    char morse_actual[21] = "";

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
                oled_limpiar();
                oled_posicionar_cursor(0, 2);
                oled_imprimir(" Volviendo al");
                oled_posicionar_cursor(0, 3);
                oled_imprimir("    menu...");
                break;
            }
            else if (lectura == SIMBOLO_PUNTO)
            {
                printf(".");
                morse_avanzar('.');
                strncat(morse_actual, ".", sizeof(morse_actual) - strlen(morse_actual) - 1);
                char buf[21];
                snprintf(buf, sizeof(buf), "Morse: %s", morse_actual);
                oled_posicionar_cursor(0, 3);
                oled_imprimir("                    ");
                oled_posicionar_cursor(0, 3);
                oled_imprimir(buf);
            }
            else if (lectura == SIMBOLO_RAYA)
            {
                printf("-");
                morse_avanzar('-');
                strncat(morse_actual, "-", sizeof(morse_actual) - strlen(morse_actual) - 1);
                char buf[21];
                snprintf(buf, sizeof(buf), "Morse: %s", morse_actual);
                oled_posicionar_cursor(0, 3);
                oled_imprimir("                    ");
                oled_posicionar_cursor(0, 3);
                oled_imprimir(buf);
            }
            else if (lectura == SIMBOLO_ESPACIO_LARGO)
            {
                char letra_final = morse_obtener_resultado();
                char buf[21];
                if (letra_final != '?' && !simbolo_desc_encontrado)
                {
                    printf(" -> [%c]\n", letra_final);
                    snprintf(buf, sizeof(buf), "-> [%c]", letra_final);
                }
                else
                {
                    printf(" -> [Símbolo no reconocido]\n");
                    snprintf(buf, sizeof(buf), "-> [?]");
                }
                oled_posicionar_cursor(0, 4);
                oled_imprimir("                    ");
                oled_posicionar_cursor(0, 4);
                oled_imprimir(buf);
                // Limpiar morse y preparar siguiente
                morse_actual[0] = '\0';
                oled_posicionar_cursor(0, 3);
                oled_imprimir("Morse:");
                simbolo_desc_encontrado = 0;
            }
            else if (lectura == SIMBOLO_DESCONOCIDO)
            {
                printf("?");
                simbolo_desc_encontrado = 1;
                strncat(morse_actual, "?", sizeof(morse_actual) - strlen(morse_actual) - 1);
                char buf[21];
                snprintf(buf, sizeof(buf), "Morse: %s", morse_actual);
                oled_posicionar_cursor(0, 3);
                oled_imprimir("                    ");
                oled_posicionar_cursor(0, 3);
                oled_imprimir(buf);
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

    // OLED:
    // [0] --- MORSEBERRY ---
    // [1]   Modo Libre
    // [2] hola mundo...
    // [3] Morse: ..
    oled_limpiar();
    oled_posicionar_cursor(10, 0);
    oled_imprimir("--- MORSEBERRY ---");
    oled_posicionar_cursor(0, 2);
    oled_imprimir("   Modo Libre");

    activar_modo_raw();
    int simbolo_desc_encontrado = 0;
    char texto_oled[21] = "";
    char morse_actual[21] = "";

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
                oled_limpiar();
                oled_posicionar_cursor(0, 2);
                oled_imprimir(" Volviendo al");
                oled_posicionar_cursor(0, 3);
                oled_imprimir("    menu...");
                break;
            }
            else if (lectura == SIMBOLO_PUNTO)
            {
                morse_avanzar('.');
                strncat(morse_actual, ".", sizeof(morse_actual) - strlen(morse_actual) - 1);
                char buf[21];
                snprintf(buf, sizeof(buf), "Morse: %.13s", morse_actual);
                oled_posicionar_cursor(0, 4);
                oled_imprimir("                    ");
                oled_posicionar_cursor(0, 4);
                oled_imprimir(buf);
            }
            else if (lectura == SIMBOLO_RAYA)
            {
                morse_avanzar('-');
                strncat(morse_actual, "-", sizeof(morse_actual) - strlen(morse_actual) - 1);
                char buf[21];
                snprintf(buf, sizeof(buf), "Morse: %.13s", morse_actual);
                oled_posicionar_cursor(0, 4);
                oled_imprimir("                    ");
                oled_posicionar_cursor(0, 4);
                oled_imprimir(buf);
            }
            else if (lectura == SIMBOLO_ESPACIO_CORTO)
            {
                char letra_final = morse_obtener_resultado();
                morse_actual[0] = '\0';
                oled_posicionar_cursor(0, 4);
                oled_imprimir("                    ");

                if (letra_final != '?' && !simbolo_desc_encontrado)
                {
                    printf("%c", tolower(letra_final));
                    int len = strlen(texto_oled);
                    if (len >= 19)
                    {
                        memmove(texto_oled, texto_oled + 1, len);
                        texto_oled[len - 1] = tolower(letra_final);
                    }
                    else
                    {
                        texto_oled[len] = tolower(letra_final);
                        texto_oled[len + 1] = '\0';
                    }
                }
                else
                {
                    printf("?");
                    int len = strlen(texto_oled);
                    if (len >= 19)
                        memmove(texto_oled, texto_oled + 1, len);
                    texto_oled[len >= 19 ? len - 1 : len] = '?';
                    texto_oled[len >= 19 ? len : len + 1] = '\0';
                }
                oled_posicionar_cursor(0, 3);
                oled_imprimir("                    ");
                oled_posicionar_cursor(0, 3);
                oled_imprimir(texto_oled);
                simbolo_desc_encontrado = 0;
            }
            else if (lectura == SIMBOLO_ESPACIO_LARGO)
            {
                printf(" ");
                int len = strlen(texto_oled);
                if (len > 0 && texto_oled[len - 1] != ' ')
                {
                    if (len >= 19)
                        memmove(texto_oled, texto_oled + 1, len);
                    texto_oled[len >= 19 ? len - 1 : len] = ' ';
                    texto_oled[len >= 19 ? len : len + 1] = '\0';
                    oled_posicionar_cursor(0, 3);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 3);
                    oled_imprimir(texto_oled);
                }
                morse_actual[0] = '\0';
                oled_posicionar_cursor(0, 4);
                oled_imprimir("                    ");
                simbolo_desc_encontrado = 0;
            }
            else if (lectura == SIMBOLO_DESCONOCIDO)
            {
                printf("?");
                simbolo_desc_encontrado = 1;
                strncat(morse_actual, "?", sizeof(morse_actual) - strlen(morse_actual) - 1);
                char buf[21];
                snprintf(buf, sizeof(buf), "Morse: %.13s", morse_actual);
                oled_posicionar_cursor(0, 4);
                oled_imprimir("                    ");
                oled_posicionar_cursor(0, 4);
                oled_imprimir(buf);
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

        // OLED:
        // [0] -- Configuracion --
        // [1] > Est. PPM
        // [2]   Est. Punto(ms)
        // [3]   Volver
        oled_limpiar();
        oled_posicionar_cursor(0, 0);
        oled_imprimir("- Configuracion -");
        {
            const char *cfg_oled[] = {"Est. PPM", "Est. Punto(ms)", "Volver"};
            for (int i = 0; i < 3; i++)
            {
                char buf[21];
                oled_posicionar_cursor(0, 2 + i);
                if ((i + 1) == opcion_resaltada)
                    snprintf(buf, sizeof(buf), "> %s", cfg_oled[i]);
                else
                    snprintf(buf, sizeof(buf), "  %s", cfg_oled[i]);
                oled_imprimir(buf);
            }
        }

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
                    if (ejecutar_opcion == 3)
                    {
                        oled_limpiar();
                        oled_posicionar_cursor(0, 2);
                        oled_imprimir(" Volviendo al");
                        oled_posicionar_cursor(0, 3);
                        oled_imprimir("    menu...");
                    }
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
            // OLED: indicar entrada de dato
            oled_limpiar();
            oled_posicionar_cursor(0, 2);
            oled_imprimir("Introduce PPM");
            oled_posicionar_cursor(0, 3);
            oled_imprimir("en SSH:");
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
            // OLED: confirmar nuevo valor
            {
                char buf[21];
                oled_limpiar();
                oled_posicionar_cursor(0, 2);
                oled_imprimir("PPM establecido:");
                snprintf(buf, sizeof(buf), "  %lld ppm", x);
                oled_posicionar_cursor(0, 3);
                oled_imprimir(buf);
            }
            sleep(1);
            activar_modo_raw();
            sleep(1);
            activar_modo_raw();
        }
        else if (ejecutar_opcion == 2)
        { // duracion punto
            restaurar_terminal();
            int punto = 0;
            printf("\n Introduce duracion punto (ms): ");
            // OLED: indicar entrada de dato
            oled_limpiar();
            oled_posicionar_cursor(0, 2);
            oled_imprimir("Introduce punto");
            oled_posicionar_cursor(0, 3);
            oled_imprimir("(ms) en SSH:");
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
            // OLED: confirmar nuevo valor
            {
                char buf[21];
                oled_limpiar();
                oled_posicionar_cursor(0, 2);
                oled_imprimir("Punto establecido:");
                snprintf(buf, sizeof(buf), "  %d ms", punto);
                oled_posicionar_cursor(0, 3);
                oled_imprimir(buf);
            }
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

    // OLED:
    // [0] --- MORSEBERRY ---
    // [1]  Prueba letras
    // [2] Escribe: [A]
    // [3] .-  [ACIERTO]
    oled_limpiar();
    oled_posicionar_cursor(10, 0);
    oled_imprimir("--- MORSEBERRY ---");
    oled_posicionar_cursor(0, 2);
    oled_imprimir(" Prueba letras");
    {
        char buf[21];
        snprintf(buf, sizeof(buf), "Escribe: [%c]", caracter_aleatorio);
        oled_posicionar_cursor(0, 3);
        oled_imprimir(buf);
    }

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
                oled_limpiar();
                oled_posicionar_cursor(0, 2);
                oled_imprimir(" Volviendo al");
                oled_posicionar_cursor(0, 3);
                oled_imprimir("    menu...");
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
                char buf_oled[21];
                if (letra_final != '?' && !simbolo_desc_encontrado && caracter_aleatorio == letra_final)
                {
                    printf(" -> [ACIERTO] - %c\n", letra_final);
                    fallado = 0;
                    snprintf(buf_oled, sizeof(buf_oled), "[ACIERTO]");
                    // OLED: acierto
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir(buf_oled);
                }
                else if (letra_final == '?' || simbolo_desc_encontrado)
                {
                    printf(" -> [FALLO] - Caracter escrito %c\n", '?');
                    num_intentos_fallidos++;
                    snprintf(buf_oled, sizeof(buf_oled), "[FALLO] %d rest.", 7 - num_intentos_fallidos);
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir(buf_oled);
                }
                else
                {
                    printf(" -> [FALLO] - Caracter escrito %c\n", letra_final);
                    num_intentos_fallidos++;
                    snprintf(buf_oled, sizeof(buf_oled), "[FALLO] %d rest.", 7 - num_intentos_fallidos);
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir(buf_oled);
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
                // OLED: nueva letra
                {
                    char buf[21];
                    snprintf(buf, sizeof(buf), "Escribe: [%c]", caracter_aleatorio);
                    oled_posicionar_cursor(0, 3);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 3);
                    oled_imprimir(buf);
                }
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

    // OLED:
    // [0] --- MORSEBERRY ---
    // [1] Escribe: abc
    // [2] Escrito: ab
    // [3] [FALLO] 5 rest.
    oled_limpiar();
    oled_posicionar_cursor(10, 0);
    oled_imprimir("--- MORSEBERRY ---");
    {
        char buf[21];
        snprintf(buf, sizeof(buf), "Escribe: %.11s", palabra_random);
        oled_posicionar_cursor(0, 2);
        oled_imprimir(buf);
        oled_posicionar_cursor(0, 3);
        oled_imprimir("Escrito: ");
    }

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
                oled_limpiar();
                oled_posicionar_cursor(0, 2);
                oled_imprimir(" Volviendo al");
                oled_posicionar_cursor(0, 3);
                oled_imprimir("    menu...");
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
                // OLED: progreso escrito en fila 3
                {
                    char buf_escrito[num + 2];
                    strncpy(buf_escrito, palabra_usuario, indice);
                    buf_escrito[indice] = '\0';
                    char buf_oled[21];
                    snprintf(buf_oled, sizeof(buf_oled), "Escrito: %.11s", buf_escrito);
                    oled_posicionar_cursor(0, 3);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 3);
                    oled_imprimir(buf_oled);
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
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir("[ACIERTO]");
                }
                else
                {
                    printf(" -> [FALLO]\n");
                    printf("         Esperado: %s\n", palabra_random);
                    printf("         Escrito : %s\n", palabra_usuario);
                    num_intentos_fallidos++;
                    char buf_oled[21];
                    snprintf(buf_oled, sizeof(buf_oled), "[FALLO] %d rest.", 7 - num_intentos_fallidos);
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir(buf_oled);
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
                // OLED: nueva palabra en fila 1, limpiar progreso
                {
                    char buf[21];
                    snprintf(buf, sizeof(buf), "Escribe: %.11s", palabra_random);
                    oled_posicionar_cursor(0, 2);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 2);
                    oled_imprimir(buf);
                    oled_posicionar_cursor(0, 3);
                    oled_imprimir("Escrito: ");
                }
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

    // OLED:
    // [0] --- MORSEBERRY ---
    // [1] Escribe: palabra
    // [2] Escrito: pal
    // [3] [FALLO] 5 rest.
    oled_limpiar();
    oled_posicionar_cursor(10, 0);
    oled_imprimir("--- MORSEBERRY ---");
    {
        char buf[21];
        snprintf(buf, sizeof(buf), "Escribe:%.12s", palabra_random);
        oled_posicionar_cursor(0, 2);
        oled_imprimir(buf);
        oled_posicionar_cursor(0, 3);
        oled_imprimir("Escrito: ");
    }

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
                oled_limpiar();
                oled_posicionar_cursor(0, 2);
                oled_imprimir(" Volviendo al");
                oled_posicionar_cursor(0, 3);
                oled_imprimir("    menu...");
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
                // OLED: progreso escrito en fila 2
                {
                    char buf_escrito[MAX_PALABRA];
                    strncpy(buf_escrito, palabra_usuario, indice);
                    buf_escrito[indice] = '\0';
                    char buf_oled[21];
                    snprintf(buf_oled, sizeof(buf_oled), "Escrito: %.11s", buf_escrito);
                    oled_posicionar_cursor(0, 3);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 3);
                    oled_imprimir(buf_oled);
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
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir("[ACIERTO]");
                }
                else
                {
                    printf(" -> [FALLO]\n");
                    printf("         Esperado: %s\n", palabra_random);
                    printf("         Escrito : %s\n", palabra_usuario);
                    num_intentos_fallidos++;
                    char buf_oled[21];
                    snprintf(buf_oled, sizeof(buf_oled), "[FALLO] %d rest.", 7 - num_intentos_fallidos);
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir(buf_oled);
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
                // OLED: nueva palabra en fila 1, limpiar progreso y resultado
                {
                    char buf[21];
                    snprintf(buf, sizeof(buf), "Escribe:%.12s", palabra_random);
                    oled_posicionar_cursor(0, 2);
                    oled_imprimir("                    ");
                    oled_posicionar_cursor(0, 2);
                    oled_imprimir(buf);
                    oled_posicionar_cursor(0, 3);
                    oled_imprimir("Escrito: ");
                    oled_posicionar_cursor(0, 4);
                    oled_imprimir("                    ");
                }
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