#include "vulkan.hpp"

#ifdef DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback( VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* )
{
    log( "\033[1;31mvalidation layer: ", callback_data->pMessage, "\033[0m" );
    return VK_FALSE;
}
#endif
