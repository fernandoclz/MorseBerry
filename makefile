# Nombre del ejecutable final
TARGET = main

# Compilador a utilizar
CC = gcc

# Banderas de compilación (Warnings activados)
CFLAGS = -Wall

# Librerías a enlazar (gpiod y pthread para los hilos)
LIBS = -lgpiod -lpthread -lm -lasound

SRC = main.c traductor_morse.c pantalla_oled.c palabras.c extern/ssd1306.c extern/ssd1306_fonts.c

# Regla principal: lo que pasa cuando solo escribes "make"
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Regla para limpiar los archivos generados
clean:
	rm -f $(TARGET)
