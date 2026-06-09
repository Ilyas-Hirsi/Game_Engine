Optional SDL2 headers for editor IntelliSense before the first CMake configure.

CMake still fetches and links SDL2 at build time via FetchContent.

To install headers here (Windows):

```powershell
Invoke-WebRequest -Uri "https://github.com/libsdl-org/SDL/releases/download/release-2.30.10/SDL2-devel-2.30.10-VC.zip" -OutFile "$env:TEMP\SDL2-devel-VC.zip"
Expand-Archive -Path "$env:TEMP\SDL2-devel-VC.zip" -DestinationPath "third_party/sdl2" -Force
```

Expected path: `third_party/sdl2/SDL2-2.30.10/include/SDL.h`
