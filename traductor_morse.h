#ifndef TRADUCTOR_MORSE_H
#define TRADUCTOR_MORSE_H
#include <stdio.h>   

// Registra un '.' o un '-' internamente sin devolver nada
void morse_avanzar(char simbolo);

// Devuelve la letra calculada hasta el momento y reinicia el estado
char morse_obtener_resultado();

void imprime_letra_como_morse(char letra);

void morse_obtener_patron(char letra, char *buffer_out, int *longitud_out);

int morse_obtener_tamano_arbol();

char morse_obtener_caracter_por_indice(int indice);

#endif