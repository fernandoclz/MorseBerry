#!/bin/bash

set -e

echo "=== Instalando dependencias de MorseBerry ==="

# Comprobar que se ejecuta como root
if [ "$EUID" -ne 0 ]; then
    echo "Ejecuta el script con sudo: sudo ./install_deps.sh"
    exit 1
fi

apt-get update

# libgpiod     -> -lgpiod
# libgpiod-dev -> cabeceras gpiod.h
# libasound2   -> -lasound (ALSA runtime)
# libasound2-dev -> cabeceras alsa/asoundlib.h
# libmath/pthread vienen incluidas en gcc/glibc, no necesitan instalación

apt-get install -y \
    gcc \
    make \
    libgpiod2 \
    libgpiod-dev \
    libasound2 \
    libasound2-dev

echo ""
echo "=== Dependencias instaladas correctamente ==="
echo "Ya puedes compilar con: make"