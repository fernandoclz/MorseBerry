#ifndef CONFIG_H
#define CONFIG_H

#include <pthread.h>

#define MAX_PALABRA 50
#define FREQ 12
#define GPIO_PREDET 17
#define GPIO_MANIP_IZQ 24
#define GPIO_MANIP_DER 23
#define NUM_MODOS_MENU 9

//tiempo por defecto -> 12ppm, palabras por minuto
#define TIEMPO_PUNTO 100    //1200/12
#define TIEMPO_RAYA 300     // 3x
#define TIEMPO_ESPACIO 700  //7x
#define DESVIACION 80       //80% de punto
#define TIEMPO_MANTENER 1400    //14x

//simbolos
#define SIMBOLO_PUNTO 1
#define SIMBOLO_RAYA 2
#define SIMBOLO_ESPACIO_CORTO 3
#define SIMBOLO_ESPACIO_LARGO 4
#define SIMBOLO_DESCONOCIDO 5
#define SIMBOLO_MANTENER_PULSADO 6

//variables globales compartidas (definidas en globals.c)
extern long long tiempo_punto;
extern long long tiempo_raya;
extern long long tiempo_espacio;
extern long long desviacion;
extern long long tiempo_mantener;

extern int morse_frecuency;
extern int morse_gpio;
extern int manip_izq_gpio;
extern int manip_der_gpio;
extern int continuar_ejecucion_hilo;
extern int sonido_activado;

extern volatile char simbolo_detectado;
extern pthread_mutex_t mutex_morse;
extern volatile __sig_atomic_t emitir_tono;

#endif // CONFIG_H