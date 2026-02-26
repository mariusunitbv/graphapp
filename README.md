<h1 align="center">
Graph Tool
<img src="https://img.shields.io/badge/language-C%2B%2B-%23f34b7d.svg" />
<img src="https://img.shields.io/badge/platform-Windows-blue" />
<img src="https://img.shields.io/badge/platform-Linux-purple" />
<img src="https://img.shields.io/badge/status-WIP-yellow" />
</h1>

> ⚠️ This project is currently a Work in Progress. Features may change. [Old version is here.](https://github.com/mariusunitbv/graphapp/tree/v1)

Tool I’ve built to create, edit, and explore graphs interactively while visualizing classic algorithms like DFS or BFS in real time. It’s designed for performance and clarity, using OpenGL for fast, hardware-accelerated rendering and Dear ImGui for a clean, responsive UI.

# Preview
<img width="1176" height="759" alt="Screenshot_20260223_230419" src="https://github.com/user-attachments/assets/e973d68f-f7b5-4b1f-a49c-c8be53018e3e" />

# Motivation
I originally built this because I wasn’t fully satisfied with the graph visualizer used in my algorithms course. I wanted something that showed algorithms step by step, exactly the way I imagine them working internally.

So I decided to make my own.

This is actually the third rewrite of the project, each time I focused more on performance and overall structure of the project. The current version is heavily optimized and can handle up to 1 billion nodes (Under 22GB, without edge storage) while keeping rendering smooth thanks to OpenGL.

It started as a learning tool, but it slowly turned into a performance challenge I genuinely enjoyed working on.

# Unit Tests
Unit tests use [Google Test](https://github.com/google/googletest) and can be run via the [GraphModelTest](https://github.com/mariusunitbv/graphapp/tree/v2/GraphTool/GraphModelTest) project or Visual Studio Test Explorer on Windows.

# Building
> ⚠️ The first build may take some time, as vcpkg needs to download and compile all dependencies locally.

## Windows
> Visual Studio includes built-in support for vcpkg, so no separate installation is required.  
> If it is not installed, it can be added from the Visual Studio Installer under `Individual components`.

This project is developed using Visual Studio 2026.
To use it with Visual Studio 2022, simply change the `Windows SDK Version` and the `Platform Toolset` in each project's Properties.

1. Open `GraphTool.slnx` in Visual Studio 2026
2. Open `Developer Command Prompt` from `Tools -> Command Line`
3. Enter `vcpkg install` to download and compile all the needed dependencies. (first time only)
4. You can now build the project normally (Build -> Build Solution or Ctrl+Shift+B).

## Linux
> On Linux, building requires [vcpkg](https://archlinux.org/packages/extra/x86_64/vcpkg/) installed from your distribution’s package manager.

During the vcpkg install process, additional system dependencies may be required.
```
cd graphapp/GraphTool && mkdir build && cd build
git clone https://github.com/microsoft/vcpkg.git
cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake ../
ninja
cd bin && ./Application
```

## Web
> Graph Tool also support building with [Emscripten](https://emscripten.org/)!

After starting the server a web page with the app is available on `http://127.0.0.1:8000/Application.html`.
```
cd graphapp/GraphTool && mkdir build && cd build
git clone https://github.com/microsoft/vcpkg.git
emsdk activate latest
emcmake cmake .. "-G" "Ninja" "-DCMAKE_MAKE_PROGRAM=/path/to/ninja" "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=%EMSDK%/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" "-DVCPKG_TARGET_TRIPLET=wasm32-emscripten" "-DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake"
cd bin
python -m http.server 8000 --bind 0.0.0.0
```

# Dependencies
All of the listed dependencies will be automatically installed when following the building steps using [vcpkg.](https://github.com/microsoft/vcpkg)

- [Dear ImGui](https://github.com/ocornut/imgui) - UI library used to build the graphical interface.
- [OpenGL 3.3](https://en.wikipedia.org/wiki/OpenGL) - Drawing backend for UI, incredibly fast and efficient GPU-based drawing.
- [SDL3](https://github.com/libsdl-org/SDL) - Handles window creation and input (keyboard, mouse, etc.). Also used as the platform backend for ImGui.
- [lodepng](https://github.com/lvandeve/lodepng) - Small helper to decode image data in memory to help in loading textures into memory.
- [glad](https://github.com/Dav1dde/glad) - Dynamic loader for modern OpenGL features.
- [GoogleTest](https://github.com/google/googletest) - Used for unit testing in Graph Model.
- [FreeType](https://github.com/freetype/freetype) - High quality font rasterer used by ImGui.
- [Emscripten](https://emscripten.org/) - Optional, only if web usage is desired.
