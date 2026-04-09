#ifndef CONFIG_H
#define CONFIG_H

#include <pthread.h>

#define MAX_PALABRA 50
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
#define SIMBOLO_ESPACIO_CORTO 3
#define SIMBOLO_ESPACIO_LARGO 4
#define SIMBOLO_DESCONOCIDO 5
#define SIMBOLO_MANTENER_PULSADO 6

// Variables globales compartidas (definidas en globals.c)
extern long long tiempo_punto;
extern long long tiempo_raya;
extern long long tiempo_espacio;
extern long long desviacion;
extern long long tiempo_mantener;

extern int morse_frecuency;
extern int morse_gpio;
extern int continuar_ejecucion_hilo;

extern volatile char simbolo_detectado;
extern pthread_mutex_t mutex_morse;
extern volatile int emitir_tono;

#endif // CONFIG_H