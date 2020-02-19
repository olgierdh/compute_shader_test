#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "logger.hpp"
#include "resource.hpp"

using sdl_context = resource< bool >;
using sdl_window  = resource< SDL_Window* >;

