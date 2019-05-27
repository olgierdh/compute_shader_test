#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <vector>

#include <stdio.h>
#include <cassert>
#include <iostream>


struct application_data
{
    SDL_Window* window  = nullptr;
    VkInstance instance = nullptr;
    std::vector< const char* > extension_names;
    std::vector< VkPhysicalDevice > physical_devices;
    std::vector< VkPhysicalDeviceProperties > device_properties;
    std::vector< VkPhysicalDeviceFeatures > device_features;
    std::vector< VkQueueFamilyProperties > queue_family_properties;
    VkPhysicalDevice selected_device = nullptr;
    VkDevice logical_device          = nullptr;
    int32_t selected_device_idx      = -1;
    int32_t selected_gfx_queue_idx   = -1;
};

constexpr int64_t aligned_size( int64_t size, int64_t alignment )
{
    return ( size + alignment - 1 ) & ~( alignment - 1 );
}

template < int64_t N > class stack_allocator
{
  public:
    constexpr stack_allocator()
        : m_current_size{0}
    {
    }
    constexpr stack_allocator( const stack_allocator& ) = delete;
    constexpr stack_allocator& operator=( const stack_allocator& ) = delete;
    constexpr stack_allocator( stack_allocator&& )                 = delete;
    constexpr stack_allocator& operator=( stack_allocator&& ) = delete;

  public:
    constexpr void* allocate( int64_t size, int64_t alignment = alignof( void* ) )
    {
        const int64_t current_address =
            reinterpret_cast< int64_t >( &m_ptr[m_current_size] );
        const int64_t aligned_address = aligned_size( current_address, alignment );
        const int64_t new_size        = ( aligned_address - current_address ) + size;
        assert( new_size < N );
        m_current_size = new_size;
        return reinterpret_cast< void* >( aligned_address );
    }

    constexpr void free( void* ptr ) {}

  private:
    char m_ptr[N];
    int64_t m_current_size;
};

template < typename T, int N, typename TAllocator > class static_array;

template < typename T, int N, typename TAllocator >
constexpr void
swap( static_array< T, N, TAllocator >& lhs, static_array< T, N, TAllocator >& rhs )
{
    using std::swap;
    swap( lhs.m_data, rhs.m_data );
    swap( lhs.m_allocator, rhs.m_allocator );
    swap( lhs.m_current_element, rhs.m_current_element );
}

template < typename T, int N, typename TAllocator > class static_array
{
  public:
    constexpr static_array( TAllocator* a )
        : m_data{nullptr}
        , m_allocator{a}
        , m_current_element{0}
    {
        assert( a != nullptr );
        m_data = reinterpret_cast< T* >( a->allocate( N * sizeof( T ), alignof( T ) ) );
    }

    ~static_array()
    {
        m_allocator->free( m_data );
        m_data            = nullptr;
        m_current_element = 0;
    }

    constexpr static_array( static_array&& other )
        : m_data{std::exchange( other.m_data, nullptr )}
        , m_allocator{std::exchange( other.m_allocator, nullptr )}
        , m_current_element{std::exchange( other.m_current_element, 0 )}
    {
    }

    constexpr static_array& operator=( static_array&& other )
    {
        using std::swap;
        static_array __tmp = std::move( other );
        swap( *this, __tmp );
        return *this;
    }

    constexpr static_array( const static_array& ) = delete;
    constexpr static_array& operator=( const static_array& ) = delete;

  public:
    constexpr friend void swap<>( static_array& lhs, static_array& rhs );

  public:
    constexpr T& operator[]( int64_t index )
    {
        assert( index < m_current_element );
        assert( index >= 0 );
        return m_data[index];
    }

    constexpr const T& operator[]( int64_t index ) const
    {
        assert( index < m_current_element );
        assert( index >= 0 );
        return m_data[index];
    }

    void push_back( T&& v )
    {
        assert( m_current_element < N );
        new ( &m_data[m_current_element] ) T{std::move( v )};
        m_current_element += 1;
    }

  private:
    T* m_data;
    TAllocator* m_allocator;
    int64_t m_current_element;
};

struct array_view
{
};

struct vk_instance
{
  public:
    vk_instance() {}

    vk_instance( vk_instance&& other )
        : m_instance{std::exchange( other.m_instance, nullptr )}
    {
    }

  private:
    VkInstance m_instance;
};

struct vk_physical_device
{
  public:
    vk_physical_device()
        : m_device( nullptr )
    {
    }

  public:
    static vk_physical_device make() { return vk_physical_device{}; }

  private:
    VkPhysicalDevice m_device;
};

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

namespace detail
{
    template < typename R, typename T, typename A0, typename... Args >
    std::vector< T >
    enumerate( R ( *fn )( A0, uint32_t*, T*, Args... ), A0 a0, Args&&... args )
    {
        std::vector< T > ret;
        uint32_t count = 0;
        fn( a0, &count, nullptr, std::forward< Args >( args )... );
        ret.resize( count );
        fn( a0, &count, ret.data(), std::forward< Args >( args )... );
        return ret;
    }
} // namespace detail

void create_vk_instance( application_data& ad )
{
    VkApplicationInfo application_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, // VkStructureType            sType
        nullptr,                            // const void                *pNext
        "Just a test",                      // const char *pApplicationName
        VK_MAKE_VERSION( 1, 0, 0 ), // uint32_t                   applicationVersion
        "Vulkan Tutorial by Intel", // const char                *pEngineName
        VK_MAKE_VERSION( 1, 0, 0 ), // uint32_t                   engineVersion
        VK_API_VERSION_1_0          // uint32_t                   apiVersion
    };

    VkInstanceCreateInfo instance_create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // VkStructureType            sType
        nullptr,                                // const void*                pNext
        0,                                      // VkInstanceCreateFlags      flags
        &application_info, // const VkApplicationInfo   *pApplicationInfo
        0,                 // uint32_t                   enabledLayerCount
        nullptr,           // const char * const        *ppEnabledLayerNames
        0,                 // uint32_t                   enabledExtensionCount
        nullptr            // const char * const        *ppEnabledExtensionNames
    };

    if ( vkCreateInstance( &instance_create_info, nullptr, &ad.instance ) != VK_SUCCESS )
    {
        std::cout << "Could not create Vulkan instance!" << std::endl;
        exit( -1 );
    }
}

void destroy_vk_instance( application_data& ad )
{
    vkDestroyInstance( ad.instance, nullptr );
    ad.instance = nullptr;
}

void enumerate_vk_devices( application_data& ad )
{
    ad.physical_devices = detail::enumerate( vkEnumeratePhysicalDevices, ad.instance );
    const auto count    = ad.physical_devices.size();

    std::cout << "Found: " << count << " physical device(s)" << std::endl;

    ad.device_properties.resize( count );
    ad.device_features.resize( count );

    for ( uint32_t i = 0; i < count; ++i )
    {
        VkPhysicalDeviceProperties& p = ad.device_properties[i];
        VkPhysicalDeviceFeatures& f   = ad.device_features[i];
        VkPhysicalDevice& d           = ad.physical_devices[i];

        vkGetPhysicalDeviceProperties( d, &p );
        vkGetPhysicalDeviceFeatures( d, &f );

        std::cout << p.deviceName << " api version: " << VK_VERSION_MAJOR( p.apiVersion )
                  << "." << VK_VERSION_MINOR( p.apiVersion ) << "."
                  << VK_VERSION_PATCH( p.apiVersion ) << std::endl;
    }
}

void enumerate_vk_extensions( application_data& ad )
{
    ad.extension_names = detail::enumerate( SDL_Vulkan_GetInstanceExtensions, ad.window );

    std::cout << "Instance extensions: " << std::endl;
    for ( const char* e : ad.extension_names )
    {
        std::cout << e << std::endl;
    }
}

void enumerate_vk_queue_families( application_data& ad )
{
    ad.queue_family_properties =
        detail::enumerate( vkGetPhysicalDeviceQueueFamilyProperties, ad.selected_device );

    for ( uint32_t i = 0; i < ad.queue_family_properties.size(); ++i )
    {
        VkQueueFamilyProperties& fp = ad.queue_family_properties[i];

        if ( ( ad.selected_gfx_queue_idx == -1 )
             && ( fp.queueFlags & VK_QUEUE_GRAPHICS_BIT ) )
        {
            ad.selected_gfx_queue_idx = i;
        }

        std::cout << "q: " << i
                  << ( ( fp.queueFlags & VK_QUEUE_GRAPHICS_BIT ) ? " [grahics]" : "" )
                  << ( ( fp.queueFlags & VK_QUEUE_TRANSFER_BIT ) ? " [transfer]" : "" )
                  << ( ( fp.queueFlags & VK_QUEUE_COMPUTE_BIT ) ? " [compute]" : "" )
                  << std::endl;
    }
}

void create_vk_logical_device( application_data& ad )
{
    std::vector< float > queue_priorities = {1.0f};

    VkDeviceQueueCreateInfo queue_create_info{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        static_cast< uint32_t >( ad.selected_gfx_queue_idx ),
        static_cast< uint32_t >( queue_priorities.size() ),
        queue_priorities.data()};

    VkDeviceCreateInfo device_create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                          nullptr,
                                          0,
                                          1,
                                          &queue_create_info,
                                          0,
                                          nullptr,
                                          0,
                                          nullptr,
                                          nullptr};

    const auto res = vkCreateDevice( ad.selected_device, &device_create_info, nullptr,
                                     &ad.logical_device );
    if ( res != VK_SUCCESS )
    {
        std::cout << "Failed to create Vulkan Device!" << std::endl;
        exit( -1 );
    }

    std::cout << "Vulkan Device created!" << std::endl;
}

void destroy_vk_logical_device( application_data& ad )
{
    vkDestroyDevice( ad.logical_device, nullptr );
    ad.logical_device = nullptr;
}

void choose_physical_device( application_data& ad )
{
    std::vector< std::tuple< uint32_t, uint32_t > > scores;

    scores.resize( ad.physical_devices.size() );

    for ( uint32_t i = 0; i < ad.physical_devices.size(); ++i )
    {
        VkPhysicalDeviceProperties& p = ad.device_properties[i];
        auto& [score, idx]            = scores[i];
        idx                           = i;

        if ( p.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
        {
            score += 10000;
        }

        score += p.limits.maxImageDimension2D;
    }

    std::sort( scores.begin(), scores.end(), []( const auto& lhs, const auto& rhs ) {
        return std::get< 0 >( lhs ) > std::get< 0 >( rhs );
    } );

    ad.selected_device     = ad.physical_devices[std::get< 1 >( scores[0] )];
    ad.selected_device_idx = std::get< 1 >( scores[0] );

    std::cout << "Selected physical device: "
              << ad.device_properties[ad.selected_device_idx].deviceName
              << " score: " << std::get< 0 >( scores[0] ) << std::endl;
}

void create_sdl_window( application_data& ad )
{
    ad.window = SDL_CreateWindow( "SDL_Test", 0, 0, 800, 600,
                                  SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN );

    if ( ad.window == nullptr )
    {
        std::cout << "Couldn't create window! %s\n" << SDL_GetError() << std::endl;
        exit( -1 );
    }
}

void destroy_sdl_window( application_data& ad )
{
    SDL_DestroyWindow( ad.window );
    ad.window = nullptr;
}

using small_static_array = static_array< int, 32, stack_allocator< 1024 > >;

int main()
{
    stack_allocator< 1024 > al;
    small_static_array sa( &al );
    small_static_array sa1( &al );

    for ( int64_t i = 0; i < 10; ++i )
    {
        sa.push_back( i );
    }

    for( int64_t i = 0; i < 10; ++i )
    {
        std::cout << sa[i] << std::endl;
    }

    sa = std::move( sa1 );

    application_data ad;

    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        std::cout << "SDL could not initialize! SDL_Error = " << SDL_GetError()
                  << std::endl;
    }
    else
    {
        create_sdl_window( ad );
        enumerate_vk_extensions( ad );
        create_vk_instance( ad );
        enumerate_vk_devices( ad );
        choose_physical_device( ad );
        enumerate_vk_queue_families( ad );
        create_vk_logical_device( ad );

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

    destroy_vk_logical_device( ad );
    destroy_vk_instance( ad );
    destroy_sdl_window( ad );
    SDL_Quit();
    
    std::cout << "End" << std::endl;

    return 0;
}
