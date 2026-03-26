#ifndef PANTALLA_OLED_H
#define PANTALLA_OLED_H

#include <stdint.h>

// Inicializa el bus I2C y manda la secuencia de arranque al SSD1306
int oled_iniciar(void);

// Limpia la pantalla (la llena de ceros)
void oled_limpiar(void);

// Posiciona el cursor. 
// X: de 0 a 127 (columnas). Y: de 0 a 7 (páginas/líneas de texto)
void oled_posicionar_cursor(uint8_t x, uint8_t y);

// Imprime una cadena de texto en la posición actual
void oled_imprimir(const char* texto);

// Cierra la comunicación
void oled_cerrar(void);

#endif