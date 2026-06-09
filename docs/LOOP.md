# Implementing the main loop

Setup (CMake, SDL2 FetchContent, Log, Timer) is already in place. You add the loop in two layers: **platform** (SDL only) and **core** (orchestration).

## Files to add

| File | Responsibility |
|------|----------------|
| `engine/platform/Window.h` | Declares `Window` — no `#include <SDL.h>` in other modules' public headers if you forward-declare SDL types |
| `engine/platform/Window.cpp` | `SDL_Init`, `SDL_CreateWindow`, `SDL_CreateRenderer`, `PollEvents`, `ShouldClose`, `BeginFrame` / `EndFrame`, `Shutdown` |
| `engine/core/Application.h` | Owns `Window` + `Timer`; declares `Run()`, `OnUpdate(float)`, `OnRender()` |
| `engine/core/Application.cpp` | Main loop body, FPS logging once per second |

## Files to edit

| File | Change |
|------|--------|
| `engine/CMakeLists.txt` | Add `EnginePlatform` library, link `SDL2::SDL2`, link `EnginePlatform` into `EngineCore` |
| `CMakeLists.txt` (root) | Uncomment the `POST_BUILD` rule to copy `SDL2.dll` next to the exe on Windows |
| `sandbox/main.cc` | Construct `engine::Application` and `return app.Run();` |

## Dependency rule

```text
sandbox/main.cc  →  EngineCore (Application)
EngineCore         →  EnginePlatform (Window)  →  SDL2
EngineCore         →  Log, Timer (no SDL)
```

`core/` must not `#include <SDL.h>`. Only `platform/Window.cpp` includes SDL.

## Loop pseudocode (`Application::Run`)

```text
Initialize window (1280×720 or similar)
timer.Reset()
fps_accumulator = 0
frame_count = 0

while not window.ShouldClose():
    dt = timer.Tick()

    window.PollEvents()      // SDL_PollEvent, set quit on SDL_QUIT / Escape
    OnUpdate(dt)             // game logic (empty at first)
    OnRender()               // window.BeginFrame() — clear + draw
    window.EndFrame()        // SDL_RenderPresent

    frame_count++
    fps_accumulator += dt
    if fps_accumulator >= 1.0:
        log FPS, dt, window size
        fps_accumulator = 0
        frame_count = 0

window.Shutdown()
```

## `Window` API (suggested)

```cpp
bool Initialize(int width, int height, const std::string& title);
void Shutdown();
void PollEvents();
bool ShouldClose() const;
void BeginFrame();   // clear framebuffer
void EndFrame();     // present
int Width() const;
int Height() const;
```

## `Application` API (suggested)

```cpp
explicit Application(std::string name);
int Run();
protected:
  virtual void OnUpdate(float delta_time);
  virtual void OnRender();
```

Start with empty `OnUpdate` / `OnRender`; first goal is a stable window and FPS in the log.

## CMake snippet (`engine/CMakeLists.txt`)

```cmake
add_library(EnginePlatform STATIC platform/Window.cpp)
target_include_directories(EnginePlatform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(EnginePlatform PUBLIC SDL2::SDL2)

add_library(EngineCore STATIC
  core/Log.cpp
  core/Timer.cpp
  core/Application.cpp
)
target_link_libraries(EngineCore PUBLIC EnginePlatform)
```

## Order of implementation

1. `Window::Initialize` / `Shutdown` — open a blank window  
2. `PollEvents` + `ShouldClose`  
3. `Application::Run` — loop with `Timer::Tick` only  
4. `BeginFrame` / `EndFrame` — clear color + present  
5. FPS log using `LogInfo` once per second  
6. `sandbox/main.cc` wires `Application::Run()`

## Verify

- Window opens and closes cleanly (X, Escape, Alt+F4)  
- Console prints `[INFO] FPS: …` about once per second  
- Rebuild from `build/` only (not the repo root)  
- On Windows, `SDL2.dll` sits beside `Game_Engine.exe` after enabling the POST_BUILD copy
