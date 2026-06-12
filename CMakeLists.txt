cmake_minimum_required(VERSION 3.20)
project(Game_Engine LANGUAGES CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not supported. Configure with:\n"
    "  cmake -S . -B build")
endif()

# SDL2 is fetched now so the platform layer can link it when you implement the loop.
include(FetchContent)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
find_package(OpenGL REQUIRED)
# Old MinGW toolchains cannot compile SDL's DirectX backend.
if(MINGW)
  set(SDL_DIRECTX OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.30.10
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL2)

FetchContent_Declare(
  SDL2_image
  GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
  GIT_TAG release-2.8.2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL2_image)

set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
set(GLAD_API "gl=4.6" CACHE STRING "" FORCE)
set(GLAD_GENERATOR "c" CACHE STRING "" FORCE)
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v0.1.36
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glad)

add_subdirectory(engine)

add_executable(Game_Engine sandbox/main.cc)
target_link_libraries(Game_Engine PRIVATE EngineCore)

if(WIN32)
  add_custom_command(TARGET Game_Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:Game_Engine>
    COMMENT "Copying SDL2.dll next to Game_Engine.exe")
  add_custom_command(TARGET Game_Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2_image::SDL2_image>
            $<TARGET_FILE_DIR:Game_Engine>
    COMMENT "Copying SDL2_image.dll next to Game_Engine.exe")
  add_custom_command(TARGET Game_Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:glad>
            $<TARGET_FILE_DIR:Game_Engine>
    COMMENT "Copying glad.dll next to Game_Engine.exe")
endif()
