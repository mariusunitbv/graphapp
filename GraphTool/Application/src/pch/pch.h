#pragma once

// C/C++ standard library headers
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <span>

#include <source_location>
#include <stdexcept>

// External library headers
#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <lodepng.h>

// Macro definitions
#define GAPP_THROW(message)                                                                   \
    throw std::runtime_error(std::string(std::source_location::current().file_name()) + ":" + \
                             std::to_string(std::source_location::current().line()) + "\n" +  \
                             std::source_location::current().function_name() + "\n" + message)
