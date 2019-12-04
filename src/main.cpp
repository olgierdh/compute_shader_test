#include <vector>

#include <stdio.h>
#include <cassert>
#include <iostream>
#include <cassert>

#include "logger.hpp"
#include "stack_allocator.hpp"
#include "static_array.hpp"
#include "application.hpp"

template < typename T, int IdBits > struct handle
{
    using value_type                      = T;
    static constexpr auto type_size       = sizeof( T );
    static constexpr auto type_bits       = type_size * 8;
    static constexpr auto id_bits         = IdBits;
    static constexpr auto generation_bits = type_bits - id_bits;

    union value_and_bits {
        value_type m_value;
        struct value
        {
            value_type m_id : id_bits;
            value_type m_generation : generation_bits;
        };
    };
};

int main()
{
    // log( "Stack memory pre-allocation for allocator: ", stack_size / ( 1024 * 1024 ),
    //      "MB" );
    {
        application app;
        app.initialize();
        app.run();
        app.deinitialize();
    }
    // application_data ad;

    // if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    // {
    //     log( "SDL could not initialize! SDL_Error = ", SDL_GetError() );
    // }
    // else
    // {
    // create_sdl_window( ad );
    // enumerate_vk_extensions( ad );
    // create_vk_instance( ad );
    // enumerate_vk_devices( ad );
    // choose_physical_device( ad );
    // enumerate_vk_queue_families( ad );
    // create_vk_logical_device( ad );
    // create_vk_queues( ad );

    // destroy_vk_logical_device( ad );
    // destroy_vk_instance( ad );
    // destroy_sdl_window( ad );
    // SDL_Quit();

    log( "End" );

    return 0;
}
