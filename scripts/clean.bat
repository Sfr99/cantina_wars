@echo off
setlocal

:: Ir a la raiz del proyecto
cd /d "%~dp0.."

echo [WINDOWS] Limpiando desde: %CD%

if exist build (
    rmdir /s /q build
    echo - Carpeta 'build' eliminada.
) else (
    echo - Carpeta 'build' no existia.
)

if exist bin (
    rmdir /s /q bin
    echo - Carpeta 'bin' eliminada.
) else (
    echo - Carpeta 'bin' no existia.
)

echo [LIMPIO] Proyecto limpio.
endlocal