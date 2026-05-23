/*

Este modulo utiliza y adapta la libreria para controladores SSD1306 
desarrollada por Aleksander Alekseev.

MIT License

Copyright (c) 2018 Aleksander Alekseev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef PANTALLA_OLED_H
#define PANTALLA_OLED_H

#include <stdint.h>

// inicializa bus I2C en linux y configura la pantalla OLED.
void oled_inicializar(void);

//limpia pantalla OLED y la deja en negro.
void oled_limpiar(void);

//posiciona cursor en la pantalla para empezar a escribir.
void oled_posicionar_cursor(uint8_t x, uint8_t y);

//escribe texto en la pantalla a partir de la posicion actual del cursor
void oled_imprimir(const char* texto);

//apaga pantalla y cierra la comunicacion I2C.
void oled_cerrar(void);

#endif // PANTALLA_OLED_H
