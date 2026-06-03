# Cantina Wars

Un juego de naves en 3D donde pilotas una nave para destruir asteroides, construido sobre un **renderizador 3D por software propio**: SDL2 solo se usa para abrir la ventana y volcar píxeles, mientras que toda la proyección en perspectiva, las transformaciones y el dibujado de geometría están implementados a mano, sin GPU ni OpenGL.

Escrito principalmente en C (con algo de C++) y compilado con CMake.

## Características

- **Renderizado 3D por software** - proyección en perspectiva, matrices de vista (`lookAt`) y transformación de mallas calculadas a mano sobre un framebuffer 2D de SDL2.
- **Cámara dinámica** que sigue a la nave desde detrás y arriba para mantener la acción siempre a la vista.
- **Geometría procedural** - la nave, las balas y los asteroides se generan como mallas a partir de sus vértices y aristas en código.
- **Mecánica de juego** - control de la nave, disparo de proyectiles con tiempo de vida, asteroides como objetivos y detección de colisiones.
- **Sistema de música** y efectos de sonido integrados.
- **Marcador de puntuaciones** persistente (`scores.txt`).

## Estructura del proyecto

```
cantina_wars/
├── assets/         # modelos (glTF), texturas y sonidos
├── include/SDL2/   # cabeceras de SDL2
├── lib/            # librerías de SDL2
├── scripts/        # scripts auxiliares
├── src/            # código fuente
├── CMakeLists.txt
└── scores.txt      # puntuaciones guardadas
```

## Compilación

Requiere CMake y un compilador de C/C++. SDL2 viene incluido en el repositorio (`include/` y `lib/`).

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

El ejecutable se genera en `bin/`.

### Linux

En Linux puede que prefieras usar las librerías SDL2 del sistema en lugar de las incluidas:

```bash
sudo dnf install SDL2-devel SDL2_image-devel SDL2_mixer-devel   # Fedora
# o
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev # Debian/Ubuntu
```

## Controles

- Mover la nave y disparar a los asteroides.
- El objetivo es sobrevivir y acumular la máxima puntuación.

## Notas técnicas

El interés principal del proyecto está en el **pipeline de renderizado**: como SDL2 no ofrece 3D nativo, todo el proceso de llevar un punto del espacio 3D a un píxel de pantalla (transformación a coordenadas de cámara, proyección en perspectiva y recorte) está implementado desde cero. Es un ejercicio práctico de los fundamentos de los gráficos por computador antes de pasar por una API como OpenGL o Vulkan.
