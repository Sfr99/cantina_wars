#!/bin/bash

# 1. Calcular la ruta raiz (un nivel arriba de donde esta este script)
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "[LINUX] Directorio raiz detectado: $ROOT_DIR"

# 2. Crear carpeta build en la raiz
mkdir -p "$ROOT_DIR/build"

# 3. Entrar y Configurar
cd "$ROOT_DIR/build" || exit

echo "[CMAKE] Configurando..."
cmake -G "MinGW Makefiles" ..
if [ $? -ne 0 ]; then
    echo "[ERROR] Fallo en cmake configuration."
    exit 1
fi

# 4. Compilar
echo "[CMAKE] Compilando..."
cmake --build .
if [ $? -ne 0 ]; then
    echo "[ERROR] Fallo en la compilacion."
    exit 1
fi

echo "-----------------------------------"
echo "[EXITO] Juego listo en bin/"
echo "-----------------------------------"