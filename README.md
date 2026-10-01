# Game_Engine

A 3D game engine and rigid-body physics simulation written from scratch in C++20, built to learn
how engines work below the level a commercial engine exposes.

Everything in `engine/` is hand-written: the entity-component system, the broad-phase BVH, the
narrow-phase collision routines, the impulse solver, the job system, and the OpenGL renderer.
The only third-party code is platform glue and math — SDL2 for windowing and input, SDL2_image for
decoding texture files, glad for GL function loading, glm for vectors and quaternions, Dear ImGui
(with ImGuiFileDialog) for the editor panels, and tinyobjloader for OBJ import. There is no physics
library and no ECS library.

**~4,900 lines of C++ across 63 files.**

## Why this project exists

I wanted to understand the parts of game development that engines normally hide: how a simulation
stays stable at a fixed timestep while rendering at a variable one, why a broad phase is a tree and
not a loop, what an impulse actually does to a body's angular velocity, and how work gets spread
across cores without locks in the hot path. The goal was never to ship a game — it was to hit the
real problems that force those answers, and to write the low-level code myself rather than call it.

The [Design notes](#design-notes) section records the problems that turned out to be genuinely
subtle.

## The demo

`sandbox/DemoScene.cpp` drops **300 compound characters — 1,800 collision primitives total** — into
a walled pen and lets them pile up. Each character is a single rigid body carrying six child shapes
(a sphere head, a capsule torso, two arms, two legs) welded to one transform, so every body is a
compound collider rather than a jointed skeleton. The floor and four walls are infinite static
half-spaces.

The camera moves with WASD / arrow keys. Ticking **Paused** in the stats panel freezes the
simulation and enables click-to-select, so you can pick any body out of the pile and edit its
components live in the inspector.

## Build

Requires CMake 3.20+ and a C++20 compiler. On Windows that means Visual Studio 2022 with the
**Desktop development with C++** workload (MSVC — the GCC 6.3 at `C:\MinGW` is far too old).
All dependencies are fetched automatically by CMake on first configure.

```powershell
cmake --preset debug
cmake --build --preset debug
.\build\Debug\Game_Engine.exe
```

Or use the helper script:

```powershell
.\scripts\build.ps1 -Run
```

On Windows, use PowerShell rather than WSL — the two use different presets and will fight over the
`build/` directory.

## Architecture

```
sandbox/          demo scene + entry point
  └── EngineCore          Application loop, Scene, ECS, systems, job system, editor panels
        └── EnginePlatform   Window, Renderer, Input, asset loading, ImGui backend
              └── SDL2, SDL2_image, glad, glm, imgui, ImGuiFileDialog, tinyobjloader
```

| Directory | Contents |
|---|---|
| `engine/core/` | Job system, timer, logging, asserts |
| `engine/Scene/` | ECS registry, scene, components, systems, collision, BVH |
| `engine/platform/` | Window, OpenGL renderer, input, mesh factory and OBJ loader |
| `engine/assets/` | Asset registry mapping string ids to GPU handles |
| `engine/ui/` | ImGui editor — hierarchy, inspector, asset browser |
| `engine/Events/` | Type-erased event bus |

### Entity-component system

`ECSRegistry` ([Registery.h](engine/Scene/Registery.h)) is a variadic template over the ten
component types the scene uses. Each type gets a `SparseSet<T>`: a dense contiguous array for
iteration plus a sparse index for O(1) lookup, with removal by swap-and-pop. Views take a parameter
pack and use a fold expression to filter the lead pool down to entities that have every requested
component, so `scene.view<TransformComponent, RigidBodyComponent>().each(...)` walks tightly packed
memory. `par_each` runs the same iteration across the job system.

Components are inert structs. All behaviour lives in `System`
([Systems.cpp](engine/Scene/Systems/Systems.cpp)).

### The loop

`Application::Run` ([Application.cpp](engine/platform/Application.cpp)) runs a fixed-timestep
accumulator: physics steps at a constant 1/60s regardless of framerate, frame time is clamped at
250ms so a stall cannot spiral, and the leftover accumulator becomes an `alpha` the renderer uses to
interpolate each body between its previous and current transform. Simulation stays deterministic and
stable; rendering stays smooth and decoupled from it.

### Physics

**Integration** — semi-implicit Euler for linear motion; angular motion integrates the quaternion
directly against the angular velocity and renormalises each step.

**Broad phase** — a dynamic AABB tree ([AABBTree.h](engine/Scene/AABBTree.h)) with
surface-area-heuristic descent for insertion, bounds fattened by a fixed margin, and rotation-based
rebalancing to keep sibling depths within one. Static and dynamic bodies live in separate trees.
`UpdateLeaf` only reinserts a body once it escapes its fattened box, so a settled pile costs almost
nothing to maintain.

**Narrow phase** — hand-written tests for every pair over a
`std::variant<Sphere, Plane, Box, Capsule>`: sphere-sphere, sphere-box, sphere-plane, box-plane,
box-box by separating-axis test over all 15 axes, capsule-plane, capsule-sphere, capsule-box, and
capsule-capsule via closest points between two segments. Dispatch happens at compile time —
`std::visit` with an `if constexpr` chain resolves the shape pair into a direct call, with no
virtual dispatch and no runtime branch table.

Contacts land in a `ContactManifold`, a fixed 8-slot heap ordered by penetration depth, so when a
compound body produces more contacts than the budget the deepest ones survive.

**Solver** — sequential impulses with full angular response. Each contact computes its effective
mass along the normal using the inertia tensor rotated into world space, applies the impulse to both
linear and angular velocity, and skips contacts that are already separating. Positional correction
accumulates the shift applied so far and subtracts its projection onto each subsequent normal, so a
body wedged in a corner is pushed out of both the floor and the wall without being double-corrected
by two contacts that share a normal.

### Job system

`TaskScheduler` ([TaskScheduler.cpp](engine/core/TaskScheduler.cpp)) is a thread pool sized to
`hardware_concurrency`, fed by a condition-variable work queue. `parallel_for` splits a range into
chunks and — rather than blocking on a barrier — has the calling thread pull tasks off the same
queue until the work is done, so the main thread contributes instead of idling.

Both collision phases run through it. Each body writes its broad-phase candidates and narrow-phase
contacts into its own slot in a pre-sized array, so workers never touch the same memory and no
locking is needed in the hot path. Contact resolution then runs serially over those buckets in a
fixed order, which keeps the result deterministic.

### Renderer

An OpenGL 3.3 core-profile forward renderer ([Renderer.cpp](engine/platform/Renderer.cpp)).
Draw calls are collected per frame and keyed by a packed `(mesh id, texture id)` pair, then each
bucket is issued as a single `glDrawElementsInstanced` with per-instance model matrices uploaded to
a shared instance VBO — a `mat4` spanning attribute locations 2 through 5 with divisor 1. The 300
demo characters collapse into one draw call. Single-mesh draws route through the same path as a
one-instance batch rather than duplicating the setup.

Each instance is frustum-culled before batching, using the six planes extracted from the
view-projection matrix tested against the mesh's transformed AABB.

Shading is currently unlit: vertex colour multiplied by an optional texture.

### Editor

A dockable ImGui layer with a scene hierarchy, a component inspector, an asset browser and a stats
panel. Entities can be created, named, selected and destroyed. Components can be added to or removed
from the selected entity, and mesh and texture fields are dropdowns populated from the asset
registry, so imported assets can be assigned without touching code. Importing a texture or an OBJ
copies the file into the project's asset folder and registers it under a root-relative id.

The inspector is driven by a small compile-time reflection table
([Reflection.h](engine/Scene/Components/Reflection.h)) — each component specialises `Reflect<T>`
with a `Fields` function that visits its members by name, so widgets are generated per field without
RTTI or macros. The same fold drives the add-component menu, which offers only the types an entity
does not already have.

Pausing the simulation enables click-to-select, which unprojects the mouse position into a world ray
and queries both BVHs plus the plane list.

## Design notes

Problems that turned out to be more subtle than expected:

- **Infinite planes cannot go in a BVH.** A half-space has no bounded AABB, so putting one in the
  tree poisons every union above it. Planes live in a separate list and are tested against each
  body's AABB with a cheap centre-and-extent half-space check before the real narrow phase runs.
- **Fattened bounds are what make a dynamic BVH viable.** Refitting a tight box every frame means
  reinserting every moving body every frame. Padding the box and only reinserting on escape turns
  the common case into a no-op.
- **Penetration correction has to accumulate.** Correcting each contact independently launches a
  body out of a corner, because the floor and the wall each push it the full depth. Tracking the
  shift applied so far and discounting it along each new normal fixes both stacking and corners.
- **A waiting thread is a wasted thread.** The first `parallel_for` blocked the main thread on the
  completion counter. Having it drain the same queue instead recovered a full core.
- **Scale and compound colliders do not mix.** Character dimensions are baked into the mesh and the
  collider, because scaling the transform would desync the two and distort the capsules — a capsule
  under non-uniform scale is no longer a capsule.

## Current limitations

Known and deliberate, listed because they define what comes next:

- **Scenes cannot be saved.** Every scene is built in code at startup by `BuildDemoScene`, so
  nothing the editor changes survives the process. Serialisation is the next structural piece of
  work, and the `Reflect<T>` table above is the hook it will use.
- **No joints or constraints.** Bodies are rigid compounds, not articulated skeletons — the demo
  characters cannot bend. A constraint solver is the main thing missing.
- **No friction.** Only normal impulses are applied, so bodies slide freely on contact.
- **No sleeping or islands.** Every dynamic body integrates and queries the tree every step, even
  once a pile has settled.
- **One solver pass, no warm starting.** Deep stacks stay visibly soft.
- **Rotation is not interpolated on render.** `previous_rotation_quat` is stored each step, but the
  render path only lerps position, so tumbling bodies judder. A `slerp` away from fixed.
- **No tests or benchmarks.**
- **Unlit shading, no shadows.**

## Roadmap

1. Scene serialisation, so authored scenes persist — and play/stop can snapshot and restore state
2. Distance and ball-socket constraints solved by sequential impulses, making real ragdolls possible
3. Normals and a directional light, replacing the current unlit shading
4. Coulomb friction in the contact solver
5. Warm starting and multiple solver iterations for stable stacking
6. Sleeping and contact islands
7. A timing harness, so performance claims are measured rather than assumed

## Troubleshooting

| Problem | Fix |
|---|---|
| `cmake` not recognised | `winget install Kitware.CMake`, then restart PowerShell |
| `Visual Studio could not find any instance` | Install the Desktop development with C++ workload |
| `make failed` | You are in WSL — use Windows PowerShell |
| Broken `build/` | `Remove-Item -Recurse -Force build`, then reconfigure |
