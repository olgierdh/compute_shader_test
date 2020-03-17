#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <algorithm>
#include <cstring>

#include "debug.hpp"
#include "static_array.hpp"
#include "vulkan_data.hpp"
#include "array_ref.hpp"

//
using names_cnt = std::vector< const char* >;

#ifdef DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback( VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                const VkDebugUtilsMessengerCallbackDataEXT*, void* );
#endif

namespace detail
{
    template < int64_t N, typename T, typename A0, typename IA0, typename TAllocator >
    void enumerate( VkResult ( *fn )( A0, uint32_t*, T* ), IA0 a0,
                    static_array< T, N, TAllocator >& a )
    {
        uint32_t count = 0;
        VkResult res   = VK_SUCCESS;

        do
        {
            res = fn( std::forward< A0 >( a0 ), &count, nullptr );

            if ( res != VK_SUCCESS )
            {
                log( "Enumerate problem!!!" );
            }

            a.resize( count );

            res = fn( std::forward< A0 >( a0 ), &count, a.data() );
        } while ( res == VK_INCOMPLETE );
    }

    template < int64_t N, typename T, typename A0, typename A1, typename IA0,
               typename IA1, typename TAllocator >
    void enumerate( VkResult ( *fn )( A0, A1, uint32_t*, T* ), IA0 a0, IA1 a1,
                    static_array< T, N, TAllocator >& a )
    {
        uint32_t count = 0;
        VkResult res   = VK_SUCCESS;

        do
        {
            res = fn( std::forward< IA0 >( a0 ), std::forward< IA1 >( a1 ), &count,
                      nullptr );

            if ( res != VK_SUCCESS )
            {
                log( "Enumerate problem!!!" );
            }

            a.resize( count );

            res = fn( std::forward< IA0 >( a0 ), std::forward< IA1 >( a1 ), &count,
                      a.data() );
        } while ( res == VK_INCOMPLETE );
    }

    template < int64_t N, typename R, typename T, typename A0, typename IA0,
               typename TAllocator >
    void enumerate( R ( *fn )( A0, uint32_t*, T* ), IA0 a0,
                    static_array< T, N, TAllocator >& a )
    {
        uint32_t count = 0;
        fn( std::forward< IA0 >( a0 ), &count, nullptr );
        a.resize( count );
        fn( std::forward< IA0 >( a0 ), &count, a.data() );
    }

    template < int64_t N, typename T, typename A0, typename IA0, typename TAllocator >
    void enumerate( void ( *fn )( A0, uint32_t*, T* ), IA0 a0,
                    static_array< T, N, TAllocator >& a )
    {
        uint32_t count = 0;
        fn( std::forward< IA0 >( a0 ), &count, nullptr );
        a.resize( count );
        fn( std::forward< IA0 >( a0 ), &count, a.data() );
    }

    inline void add_debug_extensions( names_cnt& extensions )
    {
        extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }

    inline void add_debug_layer_names( names_cnt& names )
    {
        names.push_back( "VK_LAYER_LUNARG_standard_validation" );
    }

    inline void add_required_instance_extensions( names_cnt& extensions )
    {
        extensions.push_back( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );
    }

    template < typename TAlloc >
    inline void enumerate_instance_extensions( vulkan_data< TAlloc >& vd )
    {
        detail::enumerate( vkEnumerateInstanceExtensionProperties, nullptr,
                           vd.instance_extension_properties );

        log( "Instance extensions: " );

        for ( VkExtensionProperties& e : vd.instance_extension_properties )
        {
            log( e.extensionName );
        }
    }

} // namespace detail

template < typename TAlloc, typename TCreateSurfaceF >
void initialize_vulkan( vulkan_data< TAlloc >& vd, names_cnt& required_extensions,
                        TCreateSurfaceF&& cvs, VkExtent2D expected_resolution )
{
    names_cnt layer_names{};

#ifdef DEBUG
    detail::add_debug_layer_names( layer_names );
    detail::add_debug_extensions( required_extensions );
#endif

    detail::enumerate_instance_extensions( vd );

    detail::add_required_instance_extensions( required_extensions );

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

    // now we need the surface
    create_vk_surface( vd, std::forward< TCreateSurfaceF >( cvs ) );

#ifdef DEBUG
    create_debug_utils_messanger( vd );
#endif
    enumerate_vk_devices( vd );
    choose_physical_device( vd );
    enumerate_vk_device_extensions( vd );

    const char* required_device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                VK_EXT_MEMORY_BUDGET_EXTENSION_NAME};
    check_required_device_extensions( vd, array_ref{required_device_extensions} );

    enumerate_vk_queue_families( vd );
    create_vk_logical_device( vd, array_ref{required_device_extensions} );
    create_vk_queues( vd );
    create_vk_swap_chain( vd, expected_resolution );
    create_vk_command_buffer_pool( vd );
}

template < typename TAlloc > void destroy_vulkan( vulkan_data< TAlloc >& vd )
{
    destroy_vk_command_buffer_pool( vd );
    destroy_vk_swap_chain( vd );
    destroy_vk_surface( vd );
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
        "Vulkan tutorial",                  // const char *pApplicationName
        VK_MAKE_VERSION( 1, 0, 0 ), // uint32_t                   applicationVersion
        "Vulkan tutorial",          // const char                *pEngineName
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

template < typename TAlloc, typename TCreateSurfaceF >
void create_vk_surface( vulkan_data< TAlloc >& vd, TCreateSurfaceF&& cvs )
{
    VkSurfaceKHR surface = nullptr;
    const auto ret = std::forward< TCreateSurfaceF >( cvs )( vd.instance, &surface );

    NEO_ASSERT_ALWAYS( ret, "Couldn't create vulkan surface" );

    vd.surface = surface;
}

template < typename TAlloc > void destroy_vk_surface( vulkan_data< TAlloc >& vd )
{
    vkDestroySurfaceKHR( vd.instance, vd.surface, nullptr );
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

template < typename TAlloc >
void enumerate_vk_device_extensions( vulkan_data< TAlloc >& vd )
{
    detail::enumerate( vkEnumerateDeviceExtensionProperties, vd.selected_device, nullptr,
                       vd.device_extension_properties );

    log( "Device extensions: " );

    for ( VkExtensionProperties& e : vd.device_extension_properties )
    {
        log( e.extensionName );
    }
}

template < typename TAlloc >
void check_required_device_extensions( vulkan_data< TAlloc >& vd,
                                       array_ref< const char* > required_extension )
{
    for ( const auto e : required_extension )
    {
        auto it = std::find_if( vd.device_extension_properties.begin(),
                                vd.device_extension_properties.end(),
                                [e]( const VkExtensionProperties& prop ) {
                                    return std::strcmp( e, prop.extensionName ) == 0;
                                } );
        if ( it == vd.device_extension_properties.end() )
        {
            NEO_ASSERT_ALWAYS( false, "Extension ", e, " not found!" );
        }
    }
}

template < typename TAlloc > void enumerate_vk_queue_families( vulkan_data< TAlloc >& vd )
{
    NEO_ASSERT_ALWAYS( vd.selected_device != nullptr, "Run select device first!" );

    detail::enumerate( vkGetPhysicalDeviceQueueFamilyProperties, vd.selected_device,
                       vd.queue_family_properties );

    for ( uint32_t i = 0; i < vd.queue_family_properties.size(); ++i )
    {
        VkQueueFamilyProperties& fp             = vd.queue_family_properties[i];
        VkBool32 surface_presentation_supported = false;
        const VkResult res                      = vkGetPhysicalDeviceSurfaceSupportKHR(
            vd.selected_device, i, vd.surface, &surface_presentation_supported );
        NEO_ASSERT_ALWAYS( res == VK_SUCCESS,
                           "vkGetPhysicalDeviceSurfaceSupportKHR failed!" );

        if ( vd.selected_gfx_queue_idx == -1 )
        {
            const bool has_graphics_bit =
                ( fp.queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT;

            if ( has_graphics_bit && surface_presentation_supported )
                vd.selected_gfx_queue_idx = i;
        }

        if ( ( vd.selected_compute_queue_ids == -1 )
             && ( fp.queueFlags & VK_QUEUE_COMPUTE_BIT ) == VK_QUEUE_COMPUTE_BIT
             && ( fp.queueFlags & VK_QUEUE_GRAPHICS_BIT ) != VK_QUEUE_GRAPHICS_BIT )
        {
            vd.selected_compute_queue_ids = i;
        }

        log( "q: ", i, ( ( fp.queueFlags & VK_QUEUE_GRAPHICS_BIT ) ? " [graphics]" : "" ),
             ( ( fp.queueFlags & VK_QUEUE_TRANSFER_BIT ) ? " [transfer]" : "" ),
             ( ( fp.queueFlags & VK_QUEUE_COMPUTE_BIT ) ? " [compute]" : "" ),
             ( ( surface_presentation_supported ) ? " [presentation]" : "" ) );
    }

    if ( vd.selected_compute_queue_ids == -1 )
    {
        log( "No dedicated compute queue found! Looking for combined one!" );
        for ( uint32_t i = 0; i < vd.queue_family_properties.size(); ++i )
        {
            VkQueueFamilyProperties& fp = vd.queue_family_properties[i];
            if ( ( vd.selected_compute_queue_ids == -1 )
                 && ( fp.queueFlags & VK_QUEUE_COMPUTE_BIT ) == VK_QUEUE_COMPUTE_BIT )
            {
                vd.selected_compute_queue_ids = i;
            }
        }
    }
}

template < typename TAlloc >
void create_vk_logical_device( vulkan_data< TAlloc >& vd,
                               array_ref< const char* > required_device_extensions )
{
    NEO_ASSERT_ALWAYS( vd.selected_gfx_queue_idx >= 0,
                       "Gfx queue has not been selected!" );
    NEO_ASSERT_ALWAYS( vd.selected_compute_queue_ids >= 0,
                       "Compute queue has not been selected!" );

    float queue_priorities[] = {1.0f};

    VkDeviceQueueCreateInfo queue_create_info[] = {
        {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0,
         static_cast< uint32_t >( vd.selected_gfx_queue_idx ), 1, queue_priorities},
        {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0,
         static_cast< uint32_t >( vd.selected_compute_queue_ids ), 1, queue_priorities}};

    VkDeviceCreateInfo device_create_info{
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr,
        0,
        2,
        queue_create_info,
        0,
        nullptr,
        static_cast< uint32_t >( required_device_extensions.size() ),
        required_device_extensions.raw_ptr(),
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

    vkGetDeviceQueue( vd.logical_device, vd.selected_compute_queue_ids, 0,
                      &vd.compute_queue );

    if ( vd.compute_queue == nullptr )
    {
        log( "Failed to create compute queue!" );
        exit( -1 );
    }

    log( "Compute queue created" );
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

namespace detail
{
    inline const char* vk_format_to_str( VkFormat f )
    {
        switch ( f )
        {
            case VK_FORMAT_B8G8R8A8_UNORM:
                return "VK_FORMAT_B8G8R8A8_UNORM";
            case VK_FORMAT_B8G8R8A8_SRGB:
                return "VK_FORMAT_B8G8R8A8_SRGB";
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return "VK_FORMAT_D32_SFLOAT_S8_UINT";
            case VK_FORMAT_D32_SFLOAT:
                return "VK_FORMAT_D32_SFLOAT";
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return "VK_FORMAT_D24_UNORM_S8_UINT";
            case VK_FORMAT_D16_UNORM_S8_UINT:
                return "VK_FORMAT_D16_UNORM_S8_UINT";
            case VK_FORMAT_D16_UNORM:
                return "VK_FORMAT_D16_UNORM";
            default:
                log( "id: ", static_cast< int32_t >( f ) );
                return "UNKNOWN";
        };
    }

    template < typename TAlloc >
    void create_swap_chain_synchronisation_primitives( vulkan_data< TAlloc >& vd )
    {
        VkSemaphoreCreateInfo semaphore_create_info = {
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};

        {
            VkResult res =
                vkCreateSemaphore( vd.logical_device, &semaphore_create_info, nullptr,
                                   &vd.swap_chain.image_available_semaphore );
            NEO_ASSERT_ALWAYS( res == VK_SUCCESS,
                               "Couldn't create image available semaphore" );
        }
        {
            VkResult res =
                vkCreateSemaphore( vd.logical_device, &semaphore_create_info, nullptr,
                                   &vd.swap_chain.rendering_finished_semaphore );
            NEO_ASSERT_ALWAYS( res == VK_SUCCESS,
                               "Couldn't create image rendering finished semaphore" );
        }
    }

    template < typename TAlloc >
    void acquire_surface_capabilities( vulkan_data< TAlloc >& vd )
    {
        VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            vd.selected_device, vd.surface, &vd.surface_capabilities );
        NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Couldn't acquire surface capabilities!" );
    }

    template < typename TAlloc > void acquire_surface_formats( vulkan_data< TAlloc >& vd )
    {
        detail::enumerate( vkGetPhysicalDeviceSurfaceFormatsKHR, vd.selected_device,
                           vd.surface, vd.surface_formats );

        log( "Supported surface formats:" );

        for ( VkSurfaceFormatKHR e : vd.surface_formats )
        {
            log( "\t", vk_format_to_str( e.format ) );
        }
    }

    template < typename TAlloc > void acquire_depth_format( vulkan_data< TAlloc >& vd )
    {
        // Since all depth formats may be optional, we need to find a suitable depth
        // Start with the highest precision packed format
        std::array< VkFormat, 5 > depth_formats = {
            VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM};

        for ( auto& format : depth_formats )
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties( vd.selected_device, format, &props );
            // Format must support depth stencil attachment for optimal tiling
            if ( props.optimalTilingFeatures
                 & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
            {
                vd.depth_format = format;
                log( "Supported depth format: ", vk_format_to_str( format ) );
                break;
            }
        }
    }

    template < typename TAlloc >
    void select_number_of_swap_chain_images( vulkan_data< TAlloc >& vd )
    {
        uint32_t image_count = vd.surface_capabilities.minImageCount + 1;

        if ( vd.surface_capabilities.maxImageCount > 0
             && image_count > vd.surface_capabilities.maxImageCount )
        {
            image_count = vd.surface_capabilities.maxImageCount;
        }

        vd.swap_chain.images_count = image_count;
        log( "Will be using: ", image_count, " swap chain image(s)" );
    }

    template < typename TAlloc >
    VkSurfaceFormatKHR select_swap_chain_images_format( vulkan_data< TAlloc >& vd )
    {
        if ( ( vd.surface_formats.size() == 1 )
             && ( vd.surface_formats[0].format == VK_FORMAT_UNDEFINED ) )
        {
            return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
        }

        const auto it =
            std::find_if( vd.surface_formats.begin(), vd.surface_formats.end(),
                          []( VkSurfaceFormatKHR sf ) {
                              return sf.format == VK_FORMAT_B8G8R8A8_UNORM;
                          } );

        if ( it != vd.surface_formats.end() )
        {
            return *it;
        }

        return vd.surface_formats[0];
    }

    template < typename TAlloc >
    VkExtent2D select_swap_chain_images_extent( vulkan_data< TAlloc >& vd,
                                                VkExtent2D expected_resolution )
    {
        VkExtent2D extent_2d = vd.surface_capabilities.currentExtent;

        if ( ( vd.surface_capabilities.currentExtent.width
               == std::numeric_limits< uint32_t >::max() )
             && ( vd.surface_capabilities.currentExtent.height
                  == std::numeric_limits< uint32_t >::max() ) )
        {
            extent_2d = expected_resolution;
        }

        NEO_ASSERT_ALWAYS( extent_2d.width
                               >= vd.surface_capabilities.minImageExtent.width,
                           "Expected width exceeds surface min constraint! " );
        NEO_ASSERT_ALWAYS( extent_2d.width
                               <= vd.surface_capabilities.maxImageExtent.width,
                           "Expected width exceeds surface max constraint! " );
        NEO_ASSERT_ALWAYS( extent_2d.height
                               >= vd.surface_capabilities.minImageExtent.height,
                           "Expected height exceeds surface min constraint! " );
        NEO_ASSERT_ALWAYS( extent_2d.height
                               <= vd.surface_capabilities.maxImageExtent.height,
                           "Expected height exceeds surface max constraint! " );

        return extent_2d;
    }

    template < typename TAlloc > void get_swap_chain_images( vulkan_data< TAlloc >& vd )
    {
        detail::enumerate( vkGetSwapchainImagesKHR, vd.logical_device,
                           vd.swap_chain.swap_chain, vd.swap_chain.swap_chain_images );
    }

    template < typename TAlloc >
    void create_swap_chain_image_views( vulkan_data< TAlloc >& vd )
    {
        vd.swap_chain.swap_chain_image_views.resize(
            vd.swap_chain.swap_chain_images.size() );

        std::transform( vd.swap_chain.swap_chain_images.begin(),
                        vd.swap_chain.swap_chain_images.end(),
                        vd.swap_chain.swap_chain_image_views.begin(), [&vd]( auto im ) {
                            const VkImageViewCreateInfo color_view = {
                                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                nullptr,
                                0,
                                im,
                                VK_IMAGE_VIEW_TYPE_2D,
                                vd.swap_chain.selected_format.format,
                                {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                 VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
                                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};


                            VkImageView image_view = nullptr;
                            const auto res         = vkCreateImageView(
                                vd.logical_device, &color_view, nullptr, &image_view );
                            NEO_ASSERT_ALWAYS( res == VK_SUCCESS,
                                               "Couldn't create image view!" );

                            return image_view;
                        } );
    }

    template < typename TAlloc >
    void destroy_swap_chain_image_views( vulkan_data< TAlloc >& vd )
    {
        for ( auto iv : vd.swap_chain.swap_chain_image_views )
        {
            vkDestroyImageView( vd.logical_device, iv, nullptr );
        }
    }
} // namespace detail

template < typename TAlloc >
void create_vk_swap_chain( vulkan_data< TAlloc >& vd, VkExtent2D expected_resolution )
{
    detail::create_swap_chain_synchronisation_primitives( vd );
    detail::acquire_surface_capabilities( vd );
    detail::acquire_surface_formats( vd );
    detail::acquire_depth_format( vd );
    detail::select_number_of_swap_chain_images( vd );

    vd.swap_chain.selected_format = detail::select_swap_chain_images_format( vd );
    log( "Selected swap chain format: ",
         detail::vk_format_to_str( vd.swap_chain.selected_format.format ) );

    vd.swap_chain.selected_extent =
        detail::select_swap_chain_images_extent( vd, expected_resolution );

    VkSwapchainCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                            nullptr,
                                            0,
                                            vd.surface,
                                            vd.swap_chain.images_count,
                                            vd.swap_chain.selected_format.format,
                                            vd.swap_chain.selected_format.colorSpace,
                                            vd.swap_chain.selected_extent,
                                            1,
                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                                | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                            VK_SHARING_MODE_EXCLUSIVE,
                                            0,
                                            nullptr,
                                            vd.surface_capabilities.currentTransform,
                                            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                            VK_PRESENT_MODE_IMMEDIATE_KHR,
                                            VK_TRUE,
                                            nullptr};

    {
        VkResult res = vkCreateSwapchainKHR( vd.logical_device, &create_info, nullptr,
                                             &vd.swap_chain.swap_chain );

        NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Creation of swap chain failed!" );
    }

    detail::get_swap_chain_images( vd );
    detail::create_swap_chain_image_views( vd );

    log( "Swap chain created sucesfully! " );
}

template < typename TAlloc > void destroy_vk_swap_chain( vulkan_data< TAlloc >& vd )
{
    detail::destroy_swap_chain_image_views( vd );

    vkDestroySwapchainKHR( vd.logical_device, vd.swap_chain.swap_chain, nullptr );

    vkDestroySemaphore( vd.logical_device, vd.swap_chain.image_available_semaphore,
                        nullptr );

    vkDestroySemaphore( vd.logical_device, vd.swap_chain.rendering_finished_semaphore,
                        nullptr );
}

template < typename TAlloc >
void create_vk_command_buffer_pool( vulkan_data< TAlloc >& vd )
{
    const VkCommandPoolCreateInfo cmd_pool_create_info = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        static_cast< uint32_t >( vd.selected_gfx_queue_idx )};

    const VkResult res = vkCreateCommandPool( vd.logical_device, &cmd_pool_create_info,
                                              nullptr, &vd.pool_command_buffers );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Command pool creation failed" );
}

template < typename TAlloc >
void destroy_vk_command_buffer_pool( vulkan_data< TAlloc >& vd )
{
    vkDestroyCommandPool( vd.logical_device, vd.pool_command_buffers, nullptr );
}
