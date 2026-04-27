/*

Este módulo utiliza y adapta la librería para controladores SSD1306 
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

#include "pantalla_oled.h"
#include "extern/ssd1306.h"
#include "extern/ssd1306_fonts.h"
#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

extern int i2c_fd;

// Inicializa el I2C de Linux y la pantalla
void oled_inicializar() {
    char *filename = "/dev/i2c-1"; 
    if ((i2c_fd = open(filename, O_RDWR)) < 0) {
        printf("Error al abrir el bus I2C.\n");
        return;
    }
    
    // Dirección por defecto del SSD1306 suele ser 0x3C
    if (ioctl(i2c_fd, I2C_SLAVE, 0x3C) < 0) {
        printf("Error al contactar con el dispositivo I2C.\n");
        return;
    }

    ssd1306_Init();
}

void oled_limpiar() {
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}

void oled_posicionar_cursor(uint8_t x, uint8_t y) {
    ssd1306_SetCursor(x, y * 10); 
}

void oled_imprimir(const char* texto) {
    // Usa la fuente 7x10 proporcionada en ssd1306_fonts.c
    ssd1306_WriteString((char*)texto, Font_7x10, White);
    ssd1306_UpdateScreen();
}

void oled_cerrar() {
    oled_limpiar(); // Dejamos la pantalla en negro
    if (i2c_fd >= 0) {
        close(i2c_fd); // Cerramos el bus I2C
        i2c_fd = -1;
    }
}
