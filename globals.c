#include "config.h"

long long tiempo_punto = TIEMPO_PUNTO;
long long tiempo_raya = TIEMPO_RAYA;
long long tiempo_espacio = TIEMPO_ESPACIO;
long long desviacion = DESVIACION;
long long tiempo_mantener = TIEMPO_MANTENER;

int morse_frecuency = FREQ;
int morse_gpio = GPIO_PREDET;
int continuar_ejecucion_hilo = 1;

volatile char simbolo_detectado = 0;
pthread_mutex_t mutex_morse = PTHREAD_MUTEX_INITIALIZER;
volatile int emitir_tono = 0;