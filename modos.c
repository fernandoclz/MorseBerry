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
    oled_posicionar_cursor(10, 0); // X=10, Y=0 (Fila 0)
    oled_imprimir("--- MORSEBERRY ---");

    // Textos largos para SSH (Sin límite de tamaño)
    const char *opciones_ssh[] = {
        "1. Letra a letra",
        "2. Modo libre",
        "3. Prueba letras",
        "4. Prueba conjunto de letras random",
        "5. Prueba palabras",
        "6. Configuracion",
        "7. Salir"
    };

    // Textos cortos para OLED (Máx ~14 caracteres para dejar espacio al ">")
    const char *opciones_oled[] = {
        "Letra-Letra",
        "Modo Libre",
        "Pr. Letras",
        "Pr. Conjunto",
        "Pr. Palabras",
        "Configurar",
        "Salir"
    };

    // 3. DIBUJAR OPCIONES EN CONSOLA SSH (Se muestran todas)
    for (int i = 0; i < NUM_MODOS_MENU; i++)
    {
        if ((i + 1) == opcion_resaltada)
        {
            printf(" > %s < \n", opciones_ssh[i]); // Resaltado
        }
        else
        {
            printf("   %s   \n", opciones_ssh[i]);
        }
    }

    // 4. DIBUJAR OPCIONES EN OLED (Con efecto "Scroll" y nombres cortos)
    int max_opciones_visibles_oled = 5; 
    int indice_inicio_scroll = 0;

    if (opcion_resaltada > max_opciones_visibles_oled) {
        indice_inicio_scroll = opcion_resaltada - max_opciones_visibles_oled;
    }

    char buffer[21]; // Buffer un poco más grande por seguridad (max caracteres típicos de OLED)
    int fila_oled_actual = 2; // Empezamos a pintar desde la Y=2 

    // Bucle para la porción visible de la pantalla OLED
    for (int i = indice_inicio_scroll; i < (indice_inicio_scroll + max_opciones_visibles_oled) && i < NUM_MODOS_MENU; i++)
    {
        oled_posicionar_cursor(0, fila_oled_actual);
        
        if ((i + 1) == opcion_resaltada)
        {
            // snprintf asegura que no nos pasemos de 21 caracteres
            snprintf(buffer, sizeof(buffer), "> %s", opciones_oled[i]);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "  %s", opciones_oled[i]);
        }
        
        oled_imprimir(buffer);
        fila_oled_actual++; 
    }
}

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