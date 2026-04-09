#!/bin/bash

# ==============================================================================
# Script de Configuración Inicial - MorseBerry (TFG)
# Este script instala las dependencias y configura el hardware necesario.
# ==============================================================================

# Comprobar que el script se ejecuta como superusuario (root)
if [ "$EUID" -ne 0 ]; then
  echo "❌ Error: Por favor, ejecuta este script con sudo (ej: sudo ./setup.sh)"
  exit 1
fi

echo "========================================"
echo "Iniciando configuración de MorseBerry..."
echo "========================================"

# 1. Actualizar los repositorios del sistema
echo -e "\n[1/5] Actualizando repositorios del sistema..."
apt-get update -y

# 2. Instalar herramientas de compilación y librerías
echo -e "\n[2/5] Instalando dependencias (GCC, Make, libgpiod, I2C)..."
apt-get install -y build-essential gcc make
apt-get install -y libgpiod-dev gpiod
apt-get install -y i2c-tools libi2c-dev

# 3. Habilitar la interfaz I2C en la Raspberry Pi
echo -e "\n[3/5] Habilitando el bus I2C en el hardware..."
# Usamos raspi-config en modo no interactivo para habilitarlo automáticamente
if command -v raspi-config > /dev/null; then
    raspi-config nonint do_i2c 0
    echo "I2C habilitado correctamente."
else
    echo "Advertencia: raspi-config no encontrado. Si no usas Raspberry Pi OS, habilita I2C manualmente."
fi

# 4. Configurar permisos para el usuario actual
echo -e "\n[4/5] Configurando permisos de usuario..."
# Obtenemos el usuario real que ejecutó el sudo
USUARIO=${SUDO_USER:-$USER}

# Añadimos al usuario a los grupos i2c y gpio para que no tenga que usar sudo al ejecutar el programa
usermod -aG i2c $USUARIO
usermod -aG gpio $USUARIO
echo "Usuario '$USUARIO' añadido a los grupos i2c y gpio."

# 5. Comprobación final
echo -e "\n[5/5] Comprobando dispositivos I2C conectados..."
if ls /dev/i2c* 1> /dev/null 2>&1; then
    echo "El bus I2C está activo."
else
    echo "El bus I2C se activará tras el reinicio."
fi

echo "========================================"
echo "✅ Instalación completada con éxito."
echo "⚠️  IMPORTANTE: Debes REINICIAR la Raspberry Pi para aplicar los cambios de I2C y permisos."
echo "Puedes reiniciar ahora escribiendo: sudo reboot"
echo "========================================"
