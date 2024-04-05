cmake_minimum_required(VERSION 3.0)

project(atris C)

# Definir los archivos fuente y los encabezados
set(SOURCES
    #ai.c
    atris.c
    #button.c
    #display.c
    #event.c
    #fastrand.c
    #gamemenu.c
    #grid.c
    #highscore.c
    #identity.c
    #menu.c
    #network.c
    #piece.c
    #sound.c
    #xflame.c
)

set(HEADERS
    ai.h
    atris.h
    button.h
    display.h
    fastrand.h
    grid.h
    highscore.h
    identity.h
    menu.h
    piece.h
    sound.h
)

# Agregar el ejecutable
add_executable(atris ${SOURCES} ${HEADERS})

# Configurar las bibliotecas necesarias (ajusta según sea necesario)
find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})
target_link_libraries(atris ${SDL_LIBRARY} SDL_ttf)