# Game_Engine

Development roadmap: [docs/ROADMAP.md](docs/ROADMAP.md)

## Build on Windows (PowerShell)

### Step 1 — Install CMake (done if `cmake --version` works)

```powershell
winget install Kitware.CMake
```

**Restart PowerShell** after installing so `cmake` is recognized.

### Step 2 — Install the C++ compiler (required)

You have **Visual Studio 2022 Community**, but the **C++ workload is not installed yet**.

1. Open **Start** → **Visual Studio Installer**
2. Click **Modify** on **Visual Studio Community 2022**
3. Check **Desktop development with C++**
4. Click **Install** / **Update** (several GB, one-time)
5. Restart PowerShell when it finishes

### Step 3 — Build and run

```powershell
cd "C:\Users\ilyas\OneDrive\Documentos\Game_Engine"
.\scripts\build.ps1 -Run
```

Or manually:

```powershell
cmake --preset debug
cmake --build --preset debug
.\build\Debug\Game_Engine.exe
```

---

## Do not use WSL for this project

Use **Windows PowerShell**, not the WSL/Ubuntu terminal. WSL uses different presets and often breaks the `build/` folder.

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `cmake` not recognized | Install with winget, **restart PowerShell** |
| `Visual Studio could not find any instance` | Install C++ workload (Step 2 above) |
| `make failed` | You are in WSL; use Windows PowerShell instead |
| Broken `build/` | `Remove-Item -Recurse -Force build` then reconfigure |

---

## Why MinGW does not work here

The MinGW at `C:\MinGW` is **GCC 6.3** (too old for C++17 and modern SDL2). Use Visual Studio's MSVC compiler instead.
