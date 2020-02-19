#pragma once

#include <vector>

#include "vulkan_data.hpp"
#include "application_data.hpp"

class example1 final
{
  public:
    example1( vulkan_data< application_data::stack_alloc_t >& vk_data )
        : m_vulkan_data{vk_data}
    {
    }

    static constexpr auto NAME   = "clear screen example";
    static constexpr auto WIDTH  = 1280;
    static constexpr auto HEIGHT = 720;

    void initialize();
    void step( float delta_time_ms );
    void deinitialize();

  private:
    void init_command_buffer_pool();
    void init_command_buffer();

    void record_command_buffer();

    std::vector< VkCommandBuffer > m_cmd_clear_screen;

    vulkan_data< application_data::stack_alloc_t >& m_vulkan_data;
};
