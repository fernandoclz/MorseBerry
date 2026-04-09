#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

static struct termios oldt; // Local al archivo

long long obtener_tiempo_actual() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

void activar_modo_raw() {
    struct termios newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

void restaurar_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

char generar_char_random() {
    return 'A' + rand() % 26;
}

void generar_palabra_random(int longitud, char *palabra) {
    for (int i = 0; i < longitud; i++) {
        palabra[i] = generar_char_random();
    }
    palabra[longitud] = '\0';
}