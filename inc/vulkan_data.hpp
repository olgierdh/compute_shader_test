#pragma once

#include <vulkan/vulkan.h>

#include "static_array.hpp"
#include "stack_allocator.hpp"

template < typename TAlloc > struct vulkan_data
{
    static auto constexpr data_size = 32;

    vulkan_data( TAlloc* al )
        : al{al}
        , extension_names{al}
        , physical_devices{al}
        , device_properties{al}
        , device_features{al}
        , queue_family_properties{al}
    {
    }

    VkInstance instance                      = nullptr;
    VkDebugUtilsMessengerEXT debug_messanger = nullptr;
    VkPhysicalDevice selected_device         = nullptr;
    VkDevice logical_device                  = nullptr;
    VkQueue graphics_queue                   = nullptr;
    TAlloc* al                               = nullptr;
    int32_t selected_device_idx              = -1;
    int32_t selected_gfx_queue_idx           = -1;
    int32_t selected_compute_queue_ids       = -1;
    static_array< const char*, data_size, TAlloc > extension_names;
    static_array< VkPhysicalDevice, data_size, TAlloc > physical_devices;
    static_array< VkPhysicalDeviceProperties, data_size, TAlloc > device_properties;
    static_array< VkPhysicalDeviceFeatures, data_size, TAlloc > device_features;
    static_array< VkQueueFamilyProperties, data_size, TAlloc > queue_family_properties;
};
