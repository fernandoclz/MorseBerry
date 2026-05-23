#include "traductor_morse.h"

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

    //evitar salirse del array si se introducen demsiados simobolos
    if (posicion_actual >= sizeof(arbol_morse) - 1)
    {
        posicion_actual = 0;
    }
}

char morse_obtener_resultado()
{
    char resultado = '?'; 

    if (posicion_actual > 0 && posicion_actual < sizeof(arbol_morse) - 1)
    {
        resultado = arbol_morse[posicion_actual];
    }

    //resetear pos para siguiente letra
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

    //3 imprimir
    printf("%c -> ", letra);
    for (int i = pos - 1; i >= 0; i--)
    {
        printf("%c", buffer[i]);
    }
    printf("\n");
}

// traductor_morse.c
void morse_obtener_patron(char letra, char *buffer_out, int *longitud_out)
{
    if (letra >= 'a' && letra <= 'z')
        letra = letra - 'a' + 'A';

    int tam = sizeof(arbol_morse) - 1;
    int index = -1;
    for (int i = 0; i < tam; i++) {
        if (arbol_morse[i] == letra) { index = i; break; }
    }

    *longitud_out = 0;
    if (index < 0) return;

    char tmp[16];
    int pos = 0;
    while (index > 0) {
        int parent = (index - 1) / 2;
        tmp[pos++] = (index == 2 * parent + 1) ? '.' : '-';
        index = parent;
    }
    for (int i = pos - 1; i >= 0; i--)
        buffer_out[(*longitud_out)++] = tmp[i];
}

int morse_obtener_tamano_arbol() {
    return sizeof(arbol_morse) - 1;
}

char morse_obtener_caracter_por_indice(int indice) {
    if (indice >= 0 && indice < sizeof(arbol_morse) - 1)
        return arbol_morse[indice];
    return '?';
}