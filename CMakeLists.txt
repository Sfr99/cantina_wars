cmake_minimum_required(VERSION 3.10)
project(cantina_wars)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE include)

if(WIN32)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib/windows)
    target_link_libraries(${PROJECT_NAME} PRIVATE mingw32 SDL2main SDL2 SDL2_image SDL2_mixer)
    target_link_options(${PROJECT_NAME} PRIVATE -mwindows -static-libgcc -static-libstdc++)
	target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/lib/windows
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )

elseif(UNIX AND NOT APPLE)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib/linux)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2 SDL2_image)
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,-rpath,'$ORIGIN'")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/lib/linux
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()