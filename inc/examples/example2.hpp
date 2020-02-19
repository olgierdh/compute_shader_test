#pragma once

#include <vector>

#include "glm/glm.hpp"
#include "vulkan_data.hpp"
#include "application_data.hpp"

class example2 final
{
  public:
    example2( vulkan_data< application_data::stack_alloc_t >& vk_data )
        : m_vulkan_data{vk_data}
    {
    }

    static constexpr auto NAME   = "triangle example";
    static constexpr auto WIDTH  = 1280;
    static constexpr auto HEIGHT = 720;

    void initialize();
    void step( float delta_time_ms );
    void deinitialize();

  private:
    void init_command_buffer();
    void destroy_command_buffer();

    void init_pipeline();
    void destroy_pipeline();

    void init_framebuffers_and_images();
    void destroy_framebuffers_and_images();

    void init_render_pass();
    void destroy_render_pass();

    void record_command_buffers();

    void prepare_vertices();
    void destroy_vertices();

    // one per swapchain image
    std::vector< VkCommandBuffer > m_cmd_draw;
    std::vector< VkFramebuffer > m_framebuffers;

    VkImage m_depth_stencil_image;
    VkImageView m_depth_stencil_image_view;
    VkDeviceMemory m_depth_stencil_memory;

    VkRenderPass m_render_pass;

    VkPipelineLayout m_pipeline_layout;
    VkPipeline m_pipeline;

    struct vertex
    {
        glm::vec3 position;
        glm::vec3 color;
    };

    struct
    {
        VkDeviceMemory memory;
        VkBuffer buffer;
    } vertices;

    vulkan_data< application_data::stack_alloc_t >& m_vulkan_data;
};
