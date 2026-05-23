#nombre ejecutable final
TARGET = main

#compilador usado
CC = gcc

#warnings activados
CFLAGS = -Wall

#librerias a enlazar (gpiod y pthread para los hilos)
LIBS = -lgpiod -lpthread -lm -lasound

SRC = main.c traductor_morse.c pantalla_oled.c palabras.c extern/ssd1306.c extern/ssd1306_fonts.c globals.c utils.c audio.c gpio_input.c modos.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

#limpieza
clean:
	rm -f $(TARGET)
