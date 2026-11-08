cmake_minimum_required(VERSION 3.20)
project(texel_rot CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # clangd usa isso pro autocomplete

# Sem tipo de build definido, compila otimizado (-O2) com símbolos de
# depuração (-g). Sem otimização, a conversão de quadros fica lenta demais.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "tipo de build" FORCE)
endif()

find_package(SDL3 REQUIRED)

add_executable(app
  src/main.cpp
  src/sessao.cpp
  src/emulador.cpp
  src/vram.cpp
  src/moldes.cpp
  src/efeitos.cpp
  src/roteiro.cpp
  src/tela.cpp
  src/imagem.cpp
)
target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(app PRIVATE SDL3::SDL3)

# libretro.h (interface dos cores). SYSTEM silencia avisos desse arquivo.
target_include_directories(app SYSTEM PRIVATE third_party/libretro)

# Pasta do projeto, para achar sistema/ e capturas/ de qualquer lugar.
target_compile_definitions(app PRIVATE PASTA_PROJETO="${CMAKE_SOURCE_DIR}")
