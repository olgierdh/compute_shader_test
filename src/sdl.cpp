#include "sdl.hpp"

template <> void sdl_context::deleter( bool& resource )
{
    if ( resource )
    {
        SDL_Quit();
        log( "SDL Context destroyed!" );
        resource = false;
    }
};

template <> template <> sdl_context sdl_context::make()
{
    if ( SDL_Init( 0 ) < 0 )
    {
        log( "SDL could not initialize! SDL_Error = ", SDL_GetError() );
        return sdl_context{false};
    }

    log( "SDL Context created!" );
    return sdl_context{true};
}

template <> void sdl_window::deleter( SDL_Window*& window )
{
    if ( window )
    {
        SDL_DestroyWindow( window );
        log( "SDL Window destroyed!" );
        window = nullptr;
    }
}

template <>
template <>
sdl_window sdl_window::make( char const* name, int x, int y, int width, int height )
{
    if( !SDL_WasInit( SDL_INIT_VIDEO ) )
    {
        SDL_InitSubSystem( SDL_INIT_VIDEO );
    }

    SDL_Window* w = SDL_CreateWindow( name, x, y, width, height,
                                      SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN );

    if ( w == nullptr )
    {
        log( "Couldn't create window!", SDL_GetError() );
        return {nullptr};
    }

    log( "SDL Window created!" );
    return sdl_window{w};
}
