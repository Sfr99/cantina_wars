@echo off
setlocal

:: 1. Nos movemos al directorio del script y subimos un nivel (a la raiz del proyecto)
cd /d "%~dp0.."

echo [WINDOWS] Directorio de trabajo: %CD%

:: Verificacion de seguridad
if not exist CMakeLists.txt (
    echo [ERROR] No se encuentra CMakeLists.txt en la raiz.
    echo Asegurate de ejecutar este script dentro de la carpeta scripts/.
    pause
    exit /b 1
)

:: 2. Crear carpeta build si no existe
if not exist build mkdir build

:: 3. Entrar a build
cd build

:: 4. Configurar CMake (apuntando a la raiz con "..")
echo [CMAKE] Configurando proyecto...
cmake -G "MinGW Makefiles" ..
if %errorlevel% neq 0 (
    echo [ERROR] Fallo en la configuracion de CMake.
    cd ..
    exit /b %errorlevel%
)

:: 5. Compilar
echo [CMAKE] Compilando...
cmake --build .
if %errorlevel% neq 0 (
    echo [ERROR] Fallo en la compilacion.
    cd ..
    exit /b %errorlevel%
)

echo -----------------------------------
echo [EXITO] Juego compilado correctamente.
echo -----------------------------------
cd ..
endlocal