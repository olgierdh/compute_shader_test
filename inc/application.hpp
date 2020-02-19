#pragma once

#include <charconv>
#include <array>

#include "application_data.hpp"
#include "vulkan_data.hpp"
#include "vulkan.hpp"
#include "sdl.hpp"

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

template < class TRenderer > struct application final
{
  public:
    application()
        : m_data{}
        , m_vulkan_data{&m_data.stack_alloc}
        , m_sdl_context{}
        , m_sdl_window{}
        , m_renderer{m_vulkan_data}
        , m_fps_counter{0}
        , m_time{0}
    {
    }

    application( const application& ) = delete;
    application& operator=( const application& ) = delete;
    application( application&& )                 = delete;
    application& operator=( application&& ) = delete;

  public:
    application_data& get_data() { return m_data; }

    void initialize()
    {
        m_sdl_context = sdl_context::make();
        m_sdl_window  = sdl_window::make( TRenderer::NAME, 0, 0, TRenderer::WIDTH,
                                         TRenderer::HEIGHT );

        names_cnt extensions{};
        detail::add_required_extensions( m_sdl_window.get_value(), extensions );

        // our closure for binding the window parameter
        auto create_vulkan_surface = [this]( VkInstance instance,
                                             VkSurfaceKHR* surface ) {
            const auto result =
                SDL_Vulkan_CreateSurface( m_sdl_window.get_value(), instance, surface );
            return result == SDL_TRUE;
        };

        initialize_vulkan( m_vulkan_data, extensions, create_vulkan_surface,
                           {TRenderer::WIDTH, TRenderer::HEIGHT} );

        m_renderer.initialize();
    }

    void run()
    {
        SDL_Event e{};
        bool quit = false;

        using clock_h = std::chrono::high_resolution_clock;

        auto t1 = clock_h::now();
        auto t2 = t1;

        while ( !quit )
        {
            while ( SDL_PollEvent( &e ) != 0 )
            {
                if ( e.type == SDL_QUIT )
                {
                    quit = true;
                }
            }

            t2 = clock_h::now();

            const uint64_t t_us =
                std::chrono::duration_cast< std::chrono::microseconds >( t2 - t1 )
                    .count();
            const float dt_s =
                static_cast< float >( t_us ) * 0.000001; //!< convertion to seconds

            m_renderer.step( dt_s );

            m_fps_counter += 1;
            m_time += dt_s;

            if ( m_time >= 0.5f )
            {
                update_window_name();
                m_time        = 0.0f;
                m_fps_counter = 0;
            }

            t1 = t2;
        }
    }

    void update_window_name()
    {
        std::array< char, 512 > buffer;

        const auto name_len = std::strlen( TRenderer::NAME );

        std::memcpy( buffer.data(), TRenderer::NAME, name_len );
        std::memcpy( buffer.data() + name_len, " - ", 3 );

        const float fps = m_fps_counter / m_time;
        const auto float_len =
            std::sprintf( buffer.data() + name_len + 3, "%.2f", fps );
        std::memcpy( buffer.data() + name_len + 3 + float_len, " fps", 5 );

        SDL_SetWindowTitle( m_sdl_window.get_value(), buffer.data() );
    }

    void deinitialize()
    {
        m_renderer.deinitialize();
        destroy_vulkan( m_vulkan_data );
    }


  private:
    application_data m_data;
    vulkan_data< application_data::stack_alloc_t > m_vulkan_data;
    sdl_context m_sdl_context;
    sdl_window m_sdl_window;

    TRenderer m_renderer;

    uint64_t m_fps_counter;
    float m_time;
};
