#pragma once

#include <vulkan/vulkan.h>

#include "static_array.hpp"
#include "vulkan_data.hpp"

namespace detail
{
    template < int64_t N, typename R, typename T, typename A0, typename TAllocator,
               typename... Args >
    static_array< T, N, TAllocator > enumerate( R ( *fn )( A0, uint32_t*, T*, Args... ),
                                                A0 a0, TAllocator& al, Args&&... args )
    {
        static_array< T, N, TAllocator > ret( &al );
        uint32_t count = 0;
        fn( a0, &count, nullptr, std::forward< Args >( args )... );
        ret.resize( count );
        fn( a0, &count, ret.data(), std::forward< Args >( args )... );
        return ret;
    }
} // namespace detail


template < typename TAlloc > void initialize_vulkan( vulkan_data< TAlloc >& vd )
{
    // initialize it
}

void create_vk_instance( vulkan_data& vd )
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
        log( "Could not create Vulkan instance!" );
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
    ad.physical_devices =
        detail::enumerate< 32 >( vkEnumeratePhysicalDevices, ad.instance, ad.al );
    const auto count = ad.physical_devices.size();

    log( "Found: ", count, " physical device(s)" );

    ad.device_properties.resize( count );
    ad.device_features.resize( count );

    for ( uint32_t i = 0; i < count; ++i )
    {
        VkPhysicalDeviceProperties& p = ad.device_properties[i];
        VkPhysicalDeviceFeatures& f   = ad.device_features[i];
        VkPhysicalDevice& d           = ad.physical_devices[i];

        vkGetPhysicalDeviceProperties( d, &p );
        vkGetPhysicalDeviceFeatures( d, &f );

        log( p.deviceName, " api version: ", VK_VERSION_MAJOR( p.apiVersion ), ".",
             VK_VERSION_MINOR( p.apiVersion ), ".", VK_VERSION_PATCH( p.apiVersion ) );
    }
}

void enumerate_vk_extensions( application_data& ad )
{
    ad.extension_names =
        detail::enumerate< 32 >( SDL_Vulkan_GetInstanceExtensions, ad.window, ad.al );

    log( "Instance extensions: " );
    for ( const char* e : ad.extension_names )
    {
        log( e );
    }
}

void enumerate_vk_queue_families( application_data& ad )
{
    ad.queue_family_properties = detail::enumerate< 32 >(
        vkGetPhysicalDeviceQueueFamilyProperties, ad.selected_device, ad.al );

    for ( uint32_t i = 0; i < ad.queue_family_properties.size(); ++i )
    {
        VkQueueFamilyProperties& fp = ad.queue_family_properties[i];

        if ( ( ad.selected_gfx_queue_idx == -1 )
             && ( fp.queueFlags & VK_QUEUE_GRAPHICS_BIT ) )
        {
            ad.selected_gfx_queue_idx = i;
        }

        log( "q: ", i, ( ( fp.queueFlags & VK_QUEUE_GRAPHICS_BIT ) ? " [grahics]" : "" ),
             ( ( fp.queueFlags & VK_QUEUE_TRANSFER_BIT ) ? " [transfer]" : "" ),
             ( ( fp.queueFlags & VK_QUEUE_COMPUTE_BIT ) ? " [compute]" : "" ) );
    }
}

void create_vk_logical_device( application_data& ad )
{
    float queue_priorities[] = {1.0f};

    VkDeviceQueueCreateInfo queue_create_info{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,           nullptr, 0,
        static_cast< uint32_t >( ad.selected_gfx_queue_idx ), 1,       queue_priorities};

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
        log( "Failed to create Vulkan Device!" );
        exit( -1 );
    }

    log( "Vulkan Device created!" );
}

void create_vk_queues( application_data& ad )
{
    vkGetDeviceQueue( ad.logical_device, ad.selected_gfx_queue_idx, 0,
                      &ad.graphics_queue );

    if ( ad.graphics_queue == nullptr )
    {
        log( "Failed to create graphics queue!" );
        exit( -1 );
    }

    log( "Graphics queue created" );
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

    log( "Selected physical device: ",
         ad.device_properties[ad.selected_device_idx].deviceName,
         " score: ", std::get< 0 >( scores[0] ) );
}
