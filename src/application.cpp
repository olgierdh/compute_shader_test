#include "application.hpp"

void application::initialize()
{
    m_sdl_context = sdl_context::make();
    m_sdl_window  = sdl_window::make( "Test Vulkan", 0, 0, 1280, 720 );
}

void application::run()
{
    SDL_Event e{};
    bool quit = false;

    while ( !quit )
    {
        while ( SDL_PollEvent( &e ) != 0 )
        {
            if ( e.type == SDL_QUIT )
            {
                quit = true;
            }
        }
    }
}