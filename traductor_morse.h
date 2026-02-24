#ifndef TRADUCTOR_MORSE_H
#define TRADUCTOR_MORSE_H

// Registra un '.' o un '-' internamente sin devolver nada
void morse_avanzar(char simbolo);

// Devuelve la letra calculada hasta el momento y reinicia el estado
char morse_obtener_resultado();

#endif