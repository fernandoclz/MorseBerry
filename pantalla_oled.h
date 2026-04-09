#ifndef PANTALLA_OLED_H
#define PANTALLA_OLED_H

#include <stdint.h>

/**
 * Inicializa el bus I2C en Linux y configura la pantalla OLED.
 */
void oled_inicializar(void);

/**
 * Limpia la pantalla OLED dejándola en negro.
 */
void oled_limpiar(void);

/**
 * Posiciona el cursor en la pantalla para empezar a escribir.
 * @param x Coordenada X (horizontal, típicamente de 0 a 127)
 * @param y Coordenada Y o Página (vertical)
 */
void oled_posicionar_cursor(uint8_t x, uint8_t y);

/**
 * Escribe un texto en la pantalla a partir de la posición actual del cursor
 * y actualiza la pantalla para que los cambios sean visibles.
 * @param texto Cadena de caracteres a imprimir
 */
void oled_imprimir(const char* texto);

#endif // PANTALLA_OLED_H
