#include "palabras.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_PALABRAS 20

// Puedes ampliar esta lista hasta 100 o más
static const char* lista_palabras[] = {
    "SE", "HI", "NO", "SI", "TE",
    "CASA", "PERRO", "GATO", "MESA", "SILLA",
    "COCHE", "LIBRO", "RATON", "TECLA", "PANTALLA",
    "CODIGO", "MORSE", "RADIO", "NUBE", "TIERRA"
};

const char* obtener_palabra_aleatoria( )
{
    static int inicializado = 0;
    if (!inicializado)
    {
        srand(time(NULL));
        inicializado = 1;
    }

    int numeroAleatorio = rand() % NUM_PALABRAS;

    //return lista_palabras[4]; para pruebas
    return lista_palabras[numeroAleatorio];
}