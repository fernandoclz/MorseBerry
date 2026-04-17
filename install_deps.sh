#!/bin/bash

set -e

echo "=== Instalando dependencias de MorseBerry ==="

if [ "$EUID" -ne 0 ]; then
    echo "Ejecuta el script con sudo: sudo ./install_deps.sh"
    exit 1
fi

apt-get update

ERRORES=0

instalar_paquete() {
    local paquete=$1
    echo -n "  Instalando $paquete... "
    if apt-get install -y "$paquete" > /dev/null 2>&1; then
        echo "OK"
    else
        echo "FALLO (continuando...)"
        ERRORES=$((ERRORES + 1))
    fi
}

instalar_paquete gcc
instalar_paquete make
instalar_paquete libgpiod2
instalar_paquete libgpiod-dev
instalar_paquete libasound2
instalar_paquete libasound2-dev

echo ""
if [ "$ERRORES" -eq 0 ]; then
    echo "=== Dependencias instaladas correctamente ==="
    echo "Ya puedes compilar con: make"
else
    echo "=== Instalacion completada con $ERRORES error(es) ==="
    echo "Algunos paquetes no se instalaron. Comprueba los errores arriba."
    echo "Puede que el nombre del paquete difiera en tu distribucion."
    echo "Prueba a compilar de todas formas con: make"
fi