add_library(EnginePlatform STATIC
  platform/Window.cpp
  platform/Renderer.cpp
  platform/Input/Input.cpp
)
target_include_directories(EnginePlatform PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform
)
target_link_libraries(EnginePlatform PUBLIC
  SDL2::SDL2
  SDL2_image::SDL2_image
  glad
  OpenGL::GL
  glm::glm
)

add_library(EngineCore STATIC
  core/Log.cpp
  core/Timer.cpp
  Scene/Scene.cpp
  Scene/Systems/Systems.cpp
  core/TaskScheduler.cpp
)
target_include_directories(EngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(EngineCore PUBLIC EnginePlatform)

add_library(Engine::Core ALIAS EngineCore)
add_library(Engine::Platform ALIAS EnginePlatform)
