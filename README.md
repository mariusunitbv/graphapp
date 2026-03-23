<h1 align="center">
Graph Tool
<img src="https://img.shields.io/badge/language-C%2B%2B-%23f34b7d.svg" />
<img src="https://img.shields.io/badge/platform-Windows-blue" />
<img src="https://img.shields.io/badge/platform-Linux-purple" />
<img src="https://img.shields.io/badge/platform-Web-green" />
<img src="https://img.shields.io/badge/status-WIP-yellow" />
</h1>

> ⚠️ This project is currently a Work in Progress. Features may change. [Old version is here.](https://github.com/mariusunitbv/graphapp/tree/v1)

An educational tool to explore and learn graph algorithms. Create and edit graphs, and watch algorithms like DFS and BFS run in real time. Built with OpenGL and Dear ImGui for a fast and interactive experience.

# Preview
You can run the application directly in your browser. [This](https://mariusunitbv.github.io/graphapp/) version provides the same functionality as the native C++ build, but performance may be lower due to the overhead of running WebAssembly and JavaScript in the browser environment.

<p align="center">
  <img src="https://github.com/mariusunitbv/graphapp/blob/v2/.github/images/preview.gif" alt="Preview" />
</p>

# Motivation
I originally built this because I wasn’t fully satisfied with the graph visualizer used in my algorithms course. I wanted something that showed algorithms step by step, exactly the way I imagine them working internally.
So I decided to make my own.

This is the third rewrite of the project, each time improving performance and code structure. The current version is heavily optimized, handling up to 1 billion nodes under 23 GB of RAM, with smooth rendering.

It started as a learning tool, but it slowly turned into a performance challenge I genuinely enjoyed working on.

# Features
> ⚠️ Work in Progress.

| Feature | Description |
|---------|-------------|
| Interactive graph editing | Create and modify nodes and edges easily |
| OSM parsing | Import maps from [OpenStreetMap](https://en.wikipedia.org/wiki/OpenStreetMap) |
| Save & Load | Graphs stored in compressed LZ4 format |
| Large graph support | Handles up to 1 billion nodes under 23 GB RAM |
| Fast rendering | Hardware-accelerated using OpenGL |

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
> Graph Tool also support building with [Emscripten](https://emscripten.org/)! [Ninja](https://github.com/ninja-build/ninja) will be needed.

After starting the server a web page with the app is available on `http://127.0.0.1:8000/Application.html`.
```
cd graphapp/GraphTool && mkdir build && cd build
git clone https://github.com/microsoft/vcpkg.git
emsdk activate latest
emcmake cmake .. "-G" "Ninja" "-DCMAKE_MAKE_PROGRAM=/path/to/ninja" "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=%EMSDK%/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" "-DVCPKG_TARGET_TRIPLET=wasm32-emscripten" "-DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake"
emmake ninja
cd bin
python -m http.server 8000 --bind 0.0.0.0
```

# Dependencies
All of the listed dependencies will be automatically installed when following the building steps using [vcpkg.](https://github.com/microsoft/vcpkg)

- [Dear ImGui](https://github.com/ocornut/imgui) - UI library used to build the graphical interface.
- [OpenGL 3](https://en.wikipedia.org/wiki/OpenGL) - Drawing backend for UI, incredibly fast and efficient GPU-based drawing.
- [SDL3](https://github.com/libsdl-org/SDL) - Handles window creation and input (keyboard, mouse, etc.). Also used as the platform backend for ImGui.
- [lodepng](https://github.com/lvandeve/lodepng) - Small helper to decode image data in memory to help in loading textures into memory.
- [glad](https://github.com/Dav1dde/glad) - Dynamic loader for modern OpenGL features.
- [GoogleTest](https://github.com/google/googletest) - Used for unit testing in Graph Model.
- [FreeType](https://github.com/freetype/freetype) - High quality font rasterer used by ImGui.
- [Emscripten](https://emscripten.org/) - Optional, only if web usage is desired.
- [libosmium](https://github.com/osmcode/libosmium) - Library for parsing and handling OpenStreetMap data.
- [parallel-hashmap](https://github.com/greg7mdp/parallel-hashmap) - High-performance hash map used for large-scale map parsing, much faster than `unordered_map`.
- [simdjson](https://github.com/simdjson/simdjson) - Fast JSON parser, used for saving and loading UI settings.
- [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/) - Cross-platform dialogs for selecting files and folders.
- [lz4](https://github.com/lz4/lz4) - High-speed compression library, used to compress saved graphs efficiently.
