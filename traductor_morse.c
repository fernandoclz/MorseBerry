#include "traductor_morse.h"

// Árbol binario aplanado
static const char arbol_morse[] = " ETIANMSURWDKGOHVF?L?PJBXCYZQ??54?3???2??+????16=/?????7???8?90";
static int posicion_actual = 0;

void morse_avanzar(char simbolo)
{
    if (simbolo == '.')
    {
        posicion_actual = (2 * posicion_actual) + 1;
    }
    else if (simbolo == '-')
    {
        posicion_actual = (2 * posicion_actual) + 2;
    }

    // Evitamos salirnos del array si se introducen demasiados símbolos
    if (posicion_actual >= sizeof(arbol_morse) - 1)
    {
        posicion_actual = 0;
    }
}

char morse_obtener_resultado()
{
    char resultado = '?'; // Por defecto, si hay un error o se pulsa espacio sin símbolos

    // Si hemos avanzado en el árbol y estamos dentro de los límites
    if (posicion_actual > 0 && posicion_actual < sizeof(arbol_morse) - 1)
    {
        resultado = arbol_morse[posicion_actual];
    }

    // Reseteamos la posición para la siguiente letra
    posicion_actual = 0;

    return resultado;
}