#pragma once

#include <vulkan/vulkan.h>

#include "static_array.hpp"
#include "stack_allocator.hpp"

template < typename TAlloc > struct vulkan_data
{
    vulkan_data( TAlloc* al )
        : extension_names{al}
        , physical_devices{al}
        , device_properties{al}
        , device_features{al}
        , queue_family_properties{al}
    {
    }

    VkInstance instance              = nullptr;
    VkPhysicalDevice selected_device = nullptr;
    VkDevice logical_device          = nullptr;
    VkQueue graphics_queue           = nullptr;
    int32_t selected_device_idx      = -1;
    int32_t selected_gfx_queue_idx   = -1;
    static_array< const char*, 32, TAlloc > extension_names;
    static_array< VkPhysicalDevice, 32, TAlloc > physical_devices;
    static_array< VkPhysicalDeviceProperties, 32, TAlloc > device_properties;
    static_array< VkPhysicalDeviceFeatures, 32, TAlloc > device_features;
    static_array< VkQueueFamilyProperties, 32, TAlloc > queue_family_properties;
};
