#!/bin/bash

# Script para limpiar archivos generados de documentación
# Ubicación: .github/scripts/clean_docs.sh
# Uso: .github/scripts/clean_docs.sh

set -e  # Salir si hay algún error

echo " Limpiando documentación generada..."

# Obtener la ruta del directorio del proyecto (3 niveles arriba desde .github/scripts)
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_DIR"

echo " Directorio del proyecto: $PROJECT_DIR"

# Verificar que el directorio docs existe
if [ ! -d "docs" ]; then
    echo "  Directorio docs no existe, no hay nada que limpiar"
    exit 0
fi

# Eliminar todo el contenido del directorio docs excepto archivos específicos
echo " Eliminando contenido del directorio docs..."

# Contar archivos antes de limpiar
files_before=$(find docs -type f | wc -l)
dirs_before=$(find docs -type d | wc -l)

# Eliminar todos los archivos y subdirectorios generados
# Mantener solo: .gitkeep, README.md (si existen)
find docs -mindepth 1 -maxdepth 1 ! -name ".gitkeep" ! -name "README.md" -exec rm -rf {} +

# Contar archivos después de limpiar
files_after=$(find docs -type f | wc -l)
dirs_after=$(find docs -type d | wc -l)

# Mostrar estadísticas
files_deleted=$((files_before - files_after))
dirs_deleted=$((dirs_before - dirs_after))

echo "  Eliminados: $files_deleted archivo(s) y $dirs_deleted directorio(s)"

# Verificar el estado final
if [ -d "docs" ]; then
    remaining_files=$(find docs -mindepth 1 -type f ! -name ".gitkeep" ! -name "README.md" | wc -l)
    if [ "$remaining_files" -eq 0 ]; then
        echo " Directorio docs limpiado completamente"
    else
        echo "ℹ  Quedan $remaining_files archivo(s) protegidos en docs"
    fi
fi

echo " Limpieza completada!"