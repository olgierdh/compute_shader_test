#include "application.hpp"
#include "vulkan.hpp"

namespace detail
{
    inline void add_required_extensions( SDL_Window* window, names_cnt& extensions )
    {
        unsigned int count = 0;

        assert( window );

        {
            const auto ret = SDL_Vulkan_GetInstanceExtensions( window, &count, nullptr );
            assert( ret );
        }

        {
            const auto current_size = extensions.size();
            extensions.resize( current_size + count );
            const auto ret = SDL_Vulkan_GetInstanceExtensions(
                window, &count,
                extensions.data() + sizeof( const char* ) * current_size );
            assert( ret );
        }
    }
} // namespace detail

void application::initialize()
{
    m_sdl_context = sdl_context::make();
    m_sdl_window  = sdl_window::make( "Test Vulkan", 0, 0, 1280, 720 );

    names_cnt extensions{};
    detail::add_required_extensions( m_sdl_window.get_value(), extensions );
    initialize_vulkan( m_vulkan_data, extensions );
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

void application::deinitialize() { destroy_vulkan( m_vulkan_data ); }
