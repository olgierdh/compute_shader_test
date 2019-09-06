#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "logger.hpp"
#include "resource.hpp"

using sdl_context = resource< bool >;
using sdl_window  = resource< SDL_Window* >;

// class sdl_window
// {
//   public:
//     sdl_window()
//         : m_window{nullptr}
//     {
//     }

//     explicit sdl_window( SDL_Window* window )
//         : m_window{window}
//     {
//     }

//     sdl_window( sdl_window&& other )
//         : m_window{std::exchange( other.m_window, nullptr )}
//     {
//     }

//     sdl_window& operator=( sdl_window&& other )
//     {
//         sdl_window __tmp{std::move( other )};
//         swap( *this, __tmp );
//         return *this;
//     }

//     sdl_window( const sdl_window& ) = delete;
//     sdl_window& operator=( const sdl_window& ) = delete;

//     ~sdl_window()
//     {
//         if ( m_window )
//         {
//             SDL_DestroyWindow( m_window );
//             m_window = nullptr;
//         }
//     }

//     friend void swap( sdl_window& lhs, sdl_window& rhs );

//     static sdl_window
//     make_for_vulkan( const char* const name, int x, int y, int width, int height )
//     {
//         SDL_Window* w = SDL_CreateWindow( name, x, y, width, height,
//                                           SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN );

//         if ( w == nullptr )
//         {
//             log( "Couldn't create window!", SDL_GetError() );
//             return {};
//         }

//         return sdl_window{w};
//     }

//   private:
//     SDL_Window* m_window;
// };

// void swap( sdl_window& lhs, sdl_window& rhs )
// {
//     using std::swap;
//     swap( lhs.m_window, rhs.m_window );
// }
