#pragma once

// C/C++ standard library headers
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <vector>
#include <chrono>
#include <ranges>
#include <span>

// Our own headers
#include <graph_model.h>

// External library headers
#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_surface.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <lodepng.h>
