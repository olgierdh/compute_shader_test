#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <algorithm>

#include "static_array.hpp"
#include "vulkan_data.hpp"


//
using names_cnt = std::vector< const char* >;

#ifdef DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback( VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                const VkDebugUtilsMessengerCallbackDataEXT*, void* );
#endif

namespace detail
{
    template < int64_t N, typename R, typename T, typename A0, typename TAllocator,
               typename... Args >
    void enumerate( R ( *fn )( A0, uint32_t*, T*, Args... ), A0 a0,
                    static_array< T, N, TAllocator >& a, Args&&... args )
    {
        uint32_t count = 0;
        fn( a0, &count, nullptr, std::forward< Args >( args )... );
        a.resize( count );
        fn( a0, &count, a.data(), std::forward< Args >( args )... );
    }

    template < int64_t N, typename T, typename A0, typename TAllocator, typename... Args >
    void enumerate( VkResult ( *fn )( A0, uint32_t*, T*, Args... ), A0 a0,
                    static_array< T, N, TAllocator >& a, Args&&... args )
    {
        uint32_t count = 0;
        VkResult res   = VK_SUCCESS;

        do
        {
            res = fn( a0, &count, nullptr, std::forward< Args >( args )... );

            if ( res != VK_SUCCESS )
            {
                log( "Enumerate problem!!!" );
            }

            a.resize( count );

            res = fn( a0, &count, a.data(), std::forward< Args >( args )... );
        } while ( res == VK_INCOMPLETE );
    }

    template < int64_t N, typename T, typename A0, typename TAllocator, typename... Args >
    void enumerate( void ( *fn )( A0, uint32_t*, T*, Args... ), A0 a0,
                    static_array< T, N, TAllocator >& a, Args&&... args )
    {
        uint32_t count = 0;
        fn( a0, &count, nullptr, std::forward< Args >( args )... );
        a.resize( count );
        fn( a0, &count, a.data(), std::forward< Args >( args )... );
    }

    inline void add_debug_extensions( names_cnt& extensions )
    {
        extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }

    inline void add_debug_layer_names( names_cnt& names )
    {
        names.push_back( "VK_LAYER_LUNARG_standard_validation" );
    }

} // namespace detail

template < typename TAlloc >
void initialize_vulkan( vulkan_data< TAlloc >& vd, names_cnt& required_extensions )
{
    names_cnt layer_names{};

#ifdef DEBUG
    detail::add_debug_layer_names( layer_names );
    detail::add_debug_extensions( required_extensions );
#endif

    for ( const auto& ln : layer_names )
    {
        log( "enabled layer: ", ln );
    }

    for ( const auto& re : required_extensions )
    {
        log( "required extension: ", re );
    }

    // initialize it
    create_vk_instance( vd, required_extensions, layer_names );
#ifdef DEBUG
    create_debug_utils_messanger( vd );
#endif
    enumerate_vk_devices( vd );
    choose_physical_device( vd );
    enumerate_vk_queue_families( vd );
    create_vk_logical_device( vd );
}

template < typename TAlloc > void destroy_vulkan( vulkan_data< TAlloc >& vd )
{
    destroy_vk_logical_device( vd );
#ifdef DEBUG
    destroy_debug_utils_messanger( vd );
#endif
    destroy_vk_instance( vd );
}

#ifdef DEBUG
template < typename TAlloc >
void create_debug_utils_messanger( vulkan_data< TAlloc >& vd )
{
    VkDebugUtilsMessengerCreateInfoEXT create_info = {};
    create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                              | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                              | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData       = nullptr; // Optional

    auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr(
        vd.instance, "vkCreateDebugUtilsMessengerEXT" );

    if ( func == nullptr )
    {
        log( "vkCreateDebugUtilsMessengerEXT does not exit!" );
        exit( -1 );
    }

    const auto res = func( vd.instance, &create_info, nullptr, &vd.debug_messanger );
    if ( res != VK_SUCCESS )
    {
        log( "vkCreateDebugUtilsMessengerEXT returned: ", res );
        exit( -1 );
    }
}

template < typename TAlloc >
void destroy_debug_utils_messanger( vulkan_data< TAlloc >& vd )
{
    auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr(
        vd.instance, "vkDestroyDebugUtilsMessengerEXT" );

    if ( func == nullptr )
    {
        log( "vkDestroyDebugUtilsMessengerEXT does not exit!" );
        exit( -1 );
    }

    if ( func != nullptr )
    {
        func( vd.instance, vd.debug_messanger, nullptr );
    }
}
#endif

template < typename TAlloc >
void create_vk_instance( vulkan_data< TAlloc >& vd, const names_cnt& required_extensions,
                         const names_cnt& debug_layers )
{
    VkApplicationInfo application_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, // VkStructureType            sType
        nullptr,                            // const void                *pNext
        "ChaosForge compute shader test",   // const char *pApplicationName
        VK_MAKE_VERSION( 1, 0, 0 ), // uint32_t                   applicationVersion
        "ChaosForge playground",    // const char                *pEngineName
        VK_MAKE_VERSION( 1, 0, 0 ), // uint32_t                   engineVersion
        VK_API_VERSION_1_0          // uint32_t                   apiVersion
    };

    VkInstanceCreateInfo instance_create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // VkStructureType            sType
        nullptr,                                // const void*                pNext
        0,                                      // VkInstanceCreateFlags      flags
        &application_info, // const VkApplicationInfo   *pApplicationInfo
        static_cast< uint32_t >( debug_layers.size() ), // uint32_t enabledLayerCount
        debug_layers.data(), // const char * const        *ppEnabledLayerNames
        static_cast< uint32_t >(
            required_extensions.size() ), // uint32_t enabledExtensionCount
        required_extensions.data(), // const char * const        *ppEnabledExtensionNames
    };

    if ( vkCreateInstance( &instance_create_info, nullptr, &vd.instance ) != VK_SUCCESS )
    {
        log( "Could not create Vulkan instance!" );
        exit( -1 );
    }

    log( "Vulkan instance created" );
}

template < typename TAlloc > void destroy_vk_instance( vulkan_data< TAlloc >& vd )
{
    vkDestroyInstance( vd.instance, nullptr );
    vd.instance = nullptr;
    log( "Vulkan instance destroyed" );
}

template < typename TAlloc > void enumerate_vk_devices( vulkan_data< TAlloc >& vd )
{
    detail::enumerate( vkEnumeratePhysicalDevices, vd.instance, vd.physical_devices );
    const auto count = vd.physical_devices.size();

    log( "Found: ", count, " physical device(s)" );

    vd.device_properties.resize( count );
    vd.device_features.resize( count );

    for ( uint32_t i = 0; i < count; ++i )
    {
        VkPhysicalDeviceProperties& p = vd.device_properties[i];
        VkPhysicalDeviceFeatures& f   = vd.device_features[i];
        VkPhysicalDevice& d           = vd.physical_devices[i];

        vkGetPhysicalDeviceProperties( d, &p );
        vkGetPhysicalDeviceFeatures( d, &f );

        log( p.deviceName, " api version: ", VK_VERSION_MAJOR( p.apiVersion ), ".",
             VK_VERSION_MINOR( p.apiVersion ), ".", VK_VERSION_PATCH( p.apiVersion ) );
    }
}

template < typename TAlloc, typename F >
void enumerate_vk_extensions( vulkan_data< TAlloc >& vd, F&& fnc )
{
    vd.extension_names = detail::enumerate< vd.extension_names.capacity() >(
        std::move< F >( fnc ), vd.window, vd.al );

    log( "Instance extensions: " );

    for ( const char* e : vd.extension_names )
    {
        log( e );
    }
}

template < typename TAlloc > void enumerate_vk_queue_families( vulkan_data< TAlloc >& ad )
{
    detail::enumerate( vkGetPhysicalDeviceQueueFamilyProperties, ad.selected_device,
                       ad.queue_family_properties );

    for ( uint32_t i = 0; i < ad.queue_family_properties.size(); ++i )
    {
        VkQueueFamilyProperties& fp = ad.queue_family_properties[i];

        if ( ( ad.selected_gfx_queue_idx == -1 )
             && ( fp.queueFlags & VK_QUEUE_GRAPHICS_BIT ) )
        {
            ad.selected_gfx_queue_idx = i;
        }

        if ( ( ad.selected_compute_queue_ids == -1 )
             && ( fp.queueFlags & VK_QUEUE_COMPUTE_BIT ) )
        {
            ad.selected_compute_queue_ids = i;
        }

        log( "q: ", i, ( ( fp.queueFlags & VK_QUEUE_GRAPHICS_BIT ) ? " [grahics]" : "" ),
             ( ( fp.queueFlags & VK_QUEUE_TRANSFER_BIT ) ? " [transfer]" : "" ),
             ( ( fp.queueFlags & VK_QUEUE_COMPUTE_BIT ) ? " [compute]" : "" ) );
    }
}

template < typename TAlloc > void create_vk_logical_device( vulkan_data< TAlloc >& vd )
{
    assert( vd.selected_gfx_queue_idx >= 0 );
    assert( vd.selected_compute_queue_ids >= 0 );
    assert( vd.selected_compute_queue_ids == vd.selected_gfx_queue_idx );

    float queue_priorities[] = {1.0f};

    VkDeviceQueueCreateInfo queue_create_info{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,           nullptr, 0,
        static_cast< uint32_t >( vd.selected_gfx_queue_idx ), 1,       queue_priorities};

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

    const auto res = vkCreateDevice( vd.selected_device, &device_create_info, nullptr,
                                     &vd.logical_device );
    if ( res != VK_SUCCESS )
    {
        log( "Failed to create Vulkan Device!" );
        exit( -1 );
    }

    log( "Vulkan Device created!" );
}

template < typename TAlloc > void create_vk_queues( vulkan_data< TAlloc >& vd )
{
    vkGetDeviceQueue( vd.logical_device, vd.selected_gfx_queue_idx, 0,
                      &vd.graphics_queue );

    if ( vd.graphics_queue == nullptr )
    {
        log( "Failed to create graphics queue!" );
        exit( -1 );
    }

    log( "Graphics queue created" );
}

template < typename TAlloc > void destroy_vk_logical_device( vulkan_data< TAlloc >& vd )
{
    vkDestroyDevice( vd.logical_device, nullptr );
    vd.logical_device = nullptr;
    log( "Vulkan Device destroyed!" );
}

template < typename TAlloc > void choose_physical_device( vulkan_data< TAlloc >& vd )
{
    std::vector< std::tuple< uint32_t, uint32_t > > scores;

    scores.resize( vd.physical_devices.size() );

    for ( uint32_t i = 0; i < vd.physical_devices.size(); ++i )
    {
        VkPhysicalDeviceProperties& p = vd.device_properties[i];
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

    vd.selected_device     = vd.physical_devices[std::get< 1 >( scores[0] )];
    vd.selected_device_idx = std::get< 1 >( scores[0] );

    log( "Selected physical device: ",
         vd.device_properties[vd.selected_device_idx].deviceName,
         " score: ", std::get< 0 >( scores[0] ) );
}
