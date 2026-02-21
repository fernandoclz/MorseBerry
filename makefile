# Nombre del ejecutable final
TARGET = main

# Compilador a utilizar
CC = gcc

# Banderas de compilación (Warnings activados)
CFLAGS = -Wall

# Librerías a enlazar (wiringPi es la principal)
LIBS = -lgpiod

# Archivos fuente
SRC = main.c

# Regla principal: lo que pasa cuando solo escribes "make"
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Regla para limpiar los archivos generados
clean:
	rm -f $(TARGET)

# Regla para ejecutar el programa directamente
run: $(TARGET)
	sudo ./$(TARGET)
