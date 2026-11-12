# SlopUI for C++

The C++ version of SlopUI (C++23), written by hand from the Java version (`../java`): the same classes and
names, so a change is carried over one-to-one.

| Target | What |
|---|---|
| `slopui` | The toolkit: `include/slopui/core.hpp`, `include/slopui/ui.hpp`, `src/`. No SDL, no GL. |
| `slopui_sdl` | The SDL3/OpenGL backend (OpenGL 3.3 core, or OpenGL ES 3.0 as on AmigaOS 4): `backend/`. |
| `slopui_demo` | The test program (`demo/main.cpp`). |

Build:

    cmake -S cpp -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    ninja -C build
    build/slopui_demo [--ui-size 0-4] [--theme N] [--open gallery|prefs|disks|load|editors]

See `../docs` for how to use it in a project.
