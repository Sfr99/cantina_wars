cmake_minimum_required(VERSION 3.10)
project(cantina_wars)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Recoge automáticamente todos los .cpp bajo src/ (incluyendo subdirectorios)
# CMAKE_CONFIGURE_DEPENDS fuerza reconfiguración automática al añadir/borrar ficheros
file(GLOB_RECURSE SOURCES "src/*.cpp")
set_property(DIRECTORY PROPERTY CMAKE_CONFIGURE_DEPENDS "src")
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    include        # SDL2/, SDL2_image.h, SDL2_mixer.h
    include/SDL2   # por si algún header incluye SDL.h sin prefijo de carpeta
    src            # permite includes relativos desde la raiz de src/
)

if(WIN32)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib/windows)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        mingw32
        SDL2main
        SDL2
        SDL2_image
        SDL2_mixer
    )
    # -mwindows: oculta la consola en release; quitar para ver stdout durante desarrollo
    target_link_options(${PROJECT_NAME} PRIVATE
        -mwindows
        -static-libgcc
        -static-libstdc++
    )

    # Copia todas las DLLs de lib/windows a bin/
    # IMPORTANTE: SDL2_mixer.dll debe estar en lib/windows/
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/lib/windows
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )

elseif(UNIX AND NOT APPLE)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib/linux)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL2
        SDL2_image
        SDL2_mixer
    )
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,-rpath,$ORIGIN")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/lib/linux
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()