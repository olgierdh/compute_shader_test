#pragma once

#include <cstring>
#include <fstream>
#include <vector>
#include <vulkan/vulkan.h>

#include "debug.hpp"
#include "static_array.hpp"
#include "stack_allocator.hpp"

template < typename TAlloc > struct swap_chain_data
{
    static auto constexpr data_size = 8;

    swap_chain_data( TAlloc* al )
        : swap_chain_images{al}
        , swap_chain_image_views{al} {};

    static_array< VkImage, data_size, TAlloc > swap_chain_images          = {};
    static_array< VkImageView, data_size, TAlloc > swap_chain_image_views = {};
    VkSemaphore image_available_semaphore                                 = nullptr;
    VkSemaphore rendering_finished_semaphore                              = nullptr;
    VkFence fence_cmd_finished                                            = {};
    VkSwapchainKHR swap_chain                                             = nullptr;
    uint32_t images_count                                                 = 0u;
    VkSurfaceFormatKHR selected_format = VkSurfaceFormatKHR{};
    VkExtent2D selected_extent         = VkExtent2D{};
};

template < typename TAlloc > struct vulkan_data
{
    static auto constexpr data_size                               = 32;
    static auto constexpr data_size_instance_extension_properties = 256;
    static auto constexpr data_size_device_extension_properties   = 256;

    vulkan_data( TAlloc* al )
        : al{al}
        , extension_names{al}
        , physical_devices{al}
        , device_properties{al}
        , instance_extension_properties{al}
        , device_extension_properties{al}
        , device_features{al}
        , queue_family_properties{al}
        , surface_formats{al}
        , swap_chain{al}
    {
    }

    VkInstance instance                      = nullptr;
    VkDebugUtilsMessengerEXT debug_messanger = nullptr;
    VkPhysicalDevice selected_device         = nullptr;
    VkDevice logical_device                  = nullptr;
    VkQueue graphics_queue                   = nullptr;
    VkQueue compute_queue                    = nullptr;
    VkCommandPool pool_command_buffers       = VkCommandPool{};
    VkSurfaceKHR surface                     = nullptr;
    TAlloc* al                               = nullptr;
    int32_t selected_device_idx              = -1;
    int32_t selected_gfx_queue_idx           = -1;
    int32_t selected_compute_queue_ids       = -1;
    static_array< const char*, data_size, TAlloc > extension_names;
    static_array< VkPhysicalDevice, data_size, TAlloc > physical_devices;
    static_array< VkPhysicalDeviceProperties, data_size, TAlloc > device_properties;
    static_array< VkExtensionProperties, data_size_instance_extension_properties, TAlloc >
        instance_extension_properties;
    static_array< VkExtensionProperties, data_size_device_extension_properties, TAlloc >
        device_extension_properties;
    static_array< VkPhysicalDeviceFeatures, data_size, TAlloc > device_features;
    static_array< VkQueueFamilyProperties, data_size, TAlloc > queue_family_properties;

    VkSurfaceCapabilitiesKHR surface_capabilities = VkSurfaceCapabilitiesKHR{};
    static_array< VkSurfaceFormatKHR, data_size, TAlloc > surface_formats;
    swap_chain_data< TAlloc > swap_chain;

    VkFormat depth_format;

    inline uint32_t
    get_memory_type_idx( uint32_t typeBits, VkMemoryPropertyFlags properties )
    {
        VkPhysicalDeviceMemoryProperties device_memory_properties{};
        vkGetPhysicalDeviceMemoryProperties( selected_device, &device_memory_properties );
        for ( uint32_t i = 0; i < device_memory_properties.memoryTypeCount; i++ )
        {
            if ( ( typeBits & 1 ) == 1 )
            {
                if ( ( device_memory_properties.memoryTypes[i].propertyFlags
                       & properties )
                     == properties )
                {
                    return i;
                }
            }
            typeBits >>= 1;
        }
        return 0;
    }

    VkShaderModule load_shader( const char* filename )
    {
        std::ifstream is( filename, std::ios::binary | std::ios::in | std::ios::ate );

        VkShaderModule module{};

        NEO_ASSERT_ALWAYS( is.is_open(), "File not found." );

        const size_t size = is.tellg();
        is.seekg( 0, std::ios::beg );

        std::vector< char > buffer( size );
        is.read( buffer.data(), size );
        is.close();
        assert( size > 0 );

        VkShaderModuleCreateInfo module_create_info = {
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, size,
            reinterpret_cast< uint32_t* >( buffer.data() )};

        const auto res =
            vkCreateShaderModule( logical_device, &module_create_info, NULL, &module );

        NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Creation of shader module failed. " );

        return module;
    }

    void get_memory_budget()
    {
        VkPhysicalDeviceMemoryBudgetPropertiesEXT memory_budget{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT,
            nullptr,
            {},
            {}};

        VkPhysicalDeviceMemoryProperties2 memory_properties2{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, &memory_budget, {}};

        vkGetPhysicalDeviceMemoryProperties2( selected_device, &memory_properties2 );

        for ( uint32_t i = 0; i < memory_properties2.memoryProperties.memoryHeapCount;
              ++i )
        {
            const bool is_device =
                ( ( memory_properties2.memoryProperties.memoryHeaps[i].flags
                    & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT )
                  == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT );

            log( "memory heap ", ( is_device ? "device" : "host" ) );
            log( "\t budget: ",
                 memory_budget.heapBudget[i] * ( 1.0f / ( 1024.0f * 1024.0f ) ), " MB" );
            log( "\t used: ",
                 memory_budget.heapUsage[i] * ( 1.0f / ( 1024.0f * 1024.0f ) ), " MB" );
        }
    }
};
