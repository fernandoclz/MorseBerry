#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>

#include "pantalla_oled.h"

#define I2C_ADDR 0x3C
int i2c_fd = -1;

// --- MINI FUENTE ASCII (5x8 píxeles) ---
// Contiene: Espacio(32), Números(48-57), Mayúsculas(65-90)
// En tu TFG puedes buscar "5x8 C font array" en Google y pegar el ASCII completo.
const uint8_t font_basica[128][5] = {
    [32] = {0x00, 0x00, 0x00, 0x00, 0x00}, // Espacio
    [45] = {0x08, 0x08, 0x08, 0x08, 0x08}, // Guion -
    [46] = {0x00, 0x60, 0x60, 0x00, 0x00}, // Punto .
    [48] = {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    [49] = {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    [50] = {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    [51] = {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    [52] = {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    [53] = {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    [54] = {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    [55] = {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    [56] = {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    [57] = {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    [65] = {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    [66] = {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    [67] = {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    [68] = {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    [69] = {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    [70] = {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    [71] = {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    [72] = {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    [73] = {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    [74] = {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    [75] = {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    [76] = {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    [77] = {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    [78] = {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    [79] = {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    [80] = {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    [81] = {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    [82] = {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    [83] = {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    [84] = {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    [85] = {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    [86] = {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    [87] = {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    [88] = {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    [89] = {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    [90] = {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
};

// --- FUNCIÓN DE BAJO NIVEL: Mandar comando ---
void oled_comando(uint8_t cmd) {
    uint8_t buffer[2] = {0x00, cmd}; // 0x00 indica "Comando"
    write(i2c_fd, buffer, 2);
}

// --- FUNCIÓN DE BAJO NIVEL: Mandar dato (píxeles) ---
void oled_dato(uint8_t dato) {
    uint8_t buffer[2] = {0x40, dato}; // 0x40 indica "Dato"
    write(i2c_fd, buffer, 2);
}

int oled_iniciar(void) {
    char *bus = "/dev/i2c-1";
    if ((i2c_fd = open(bus, O_RDWR)) < 0) {
        perror("Error al abrir bus I2C");
        return -1;
    }
    if (ioctl(i2c_fd, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("Error al contactar con la direccion OLED");
        return -1;
    }

    // Secuencia mágica de inicialización (Directo del Datasheet del SSD1306)
    oled_comando(0xAE); // Display OFF
    oled_comando(0xD5); oled_comando(0x80); // Set clock
    oled_comando(0xA8); oled_comando(0x3F); // Set multiplex ratio (64 px)
    oled_comando(0xD3); oled_comando(0x00); // Set display offset
    oled_comando(0x40);                     // Set start line 0
    oled_comando(0x8D); oled_comando(0x14); // Enable charge pump (muy importante para encender los leds)
    oled_comando(0x20); oled_comando(0x02); // Addressing Mode: Page Mode
    oled_comando(0xA1);                     // Flip horizontal
    oled_comando(0xC8);                     // Flip vertical
    oled_comando(0xDA); oled_comando(0x12); // COM pins config
    oled_comando(0x81); oled_comando(0xCF); // Contraste
    oled_comando(0xD9); oled_comando(0xF1); // Pre-charge
    oled_comando(0xDB); oled_comando(0x40); // VCOMH deselect
    oled_comando(0xA4);                     // Resume display
    oled_comando(0xA6);                     // Display Normal (no invertido)
    oled_comando(0xAF);                     // DISPLAY ON!

    oled_limpiar();
    return 0;
}

void oled_posicionar_cursor(uint8_t x, uint8_t y) {
    oled_comando(0xB0 + (y & 0x07));      // Set Page Start Address (0-7)
    oled_comando(0x00 + (x & 0x0F));      // Set Lower Column Address
    oled_comando(0x10 + ((x >> 4) & 0x0F)); // Set Higher Column Address
}

void oled_limpiar(void) {
    for (uint8_t pagina = 0; pagina < 8; pagina++) {
        oled_posicionar_cursor(0, pagina);
        for (uint8_t columna = 0; columna < 128; columna++) {
            oled_dato(0x00); // Apagar todos los píxeles de la columna
        }
    }
}

void oled_imprimir(const char* texto) {
    while (*texto) {
        uint8_t char_idx = (uint8_t)(*texto);
        
        // Si el caracter está en nuestra fuente, lo dibujamos
        if (char_idx < 128) {
            for (int i = 0; i < 5; i++) {
                oled_dato(font_basica[char_idx][i]); // Imprimir los 5 bytes de la letra
            }
            oled_dato(0x00); // Un byte de espacio entre letras
        }
        texto++;
    }
}

void oled_cerrar(void) {
    if (i2c_fd >= 0) {
        oled_limpiar();
        oled_comando(0xAE); // Display OFF
        close(i2c_fd);
    }
}