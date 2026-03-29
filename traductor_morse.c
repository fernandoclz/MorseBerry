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

void imprime_letra_como_morse(char letra){
    if (letra >= 'a' && letra <= 'z')
        letra = letra - 'a' + 'A';

    int tam = sizeof(arbol_morse) - 1;
    int index = -1;

    // buscar letra en arbol
    int encontrado = 0;
    int i = 0;
    while(i < tam && !encontrado){
        if (arbol_morse[i] == letra)
        {
            index = i;
            encontrado = 1;
        }

        i++;
    }

    if (index == -1)
    {
        printf("Caracter no encontrado\n");
        return;
    }

    //reconstruir camino del arbol
    char buffer[16];
    int pos = 0;

    while (index > 0)
    {
        int parent = (index - 1) / 2;

        if (index == (2 * parent + 1))
            buffer[pos++] = '.'; // izquierda
        else
            buffer[pos++] = '-'; // derecha

        index = parent;
    }

    // 3. Imprimir en orden correcto (invertido)
    printf("%c -> ", letra);
    for (int i = pos - 1; i >= 0; i--)
    {
        printf("%c", buffer[i]);
    }
    printf("\n");
}