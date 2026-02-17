#!/bin/bash

# Calcular raiz
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "[LINUX] Limpiando desde: $ROOT_DIR"

rm -rf "$ROOT_DIR/build"
echo "- Carpeta 'build' eliminada."

rm -rf "$ROOT_DIR/bin"
echo "- Carpeta 'bin' eliminada."

echo "[LIMPIO] Listo."