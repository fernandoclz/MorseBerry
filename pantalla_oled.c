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
    char *filename = "/dev/i2c-1"; // Bus I2C estándar en Raspberry Pi
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
    // Si tus "Y" en main.c son "páginas" (0 a 7), multiplícalo por 10 (altura de fuente) o usa y directo.
    ssd1306_SetCursor(x, y * 10); 
}

void oled_imprimir(const char* texto) {
    // Usa la fuente 7x10 proporcionada en ssd1306_fonts.c
    ssd1306_WriteString((char*)texto, Font_7x10, White);
    ssd1306_UpdateScreen();
}
