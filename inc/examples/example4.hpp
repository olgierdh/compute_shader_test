#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "vulkan_data.hpp"
#include "application_data.hpp"
#include "model.hpp"
#include "image.hpp"

class example4 final
{
  public:
    example4( vulkan_data< application_data::stack_alloc_t >& vk_data )
        : m_vulkan_data{vk_data}
    {
    }

    static constexpr auto NAME   = "cat example";
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

    void create_uniform_buffers();
    void destroy_uniform_buffers();

    void init_descriptor_set_layout();
    void destroy_descriptor_set_layout();

    void create_descriptor_pool();
    void destroy_descriptor_pool();

    void create_descriptor_sets();
    void destroy_descriptor_sets();

    void init_framebuffers_and_images();
    void destroy_framebuffers_and_images();

    void init_render_pass();
    void destroy_render_pass();

    void record_command_buffers();

    void create_vertex_buffer( const std::vector< vtx_t::vertex >& vertices );
    void destroy_vertex_buffer();

    void create_index_buffer( const std::vector< vtx_t::index >& indices );
    void destroy_index_buffer();

    void create_texture( const image& img );
    void copy_texture_data( const image& img );
    void destroy_texture();

    void update_unform_buffer( float dt_s, uint32_t current_image );

    // one per swapchain image
    std::vector< VkCommandBuffer > m_cmd_draw;
    std::vector< VkFramebuffer > m_framebuffers;

    VkImage m_depth_stencil_image;
    VkImageView m_depth_stencil_image_view;
    VkDeviceMemory m_depth_stencil_memory;

    VkRenderPass m_render_pass;

    VkDescriptorSetLayout m_descriptor_set_layout;
    VkDescriptorPool m_descriptor_pool;
    std::vector< VkDescriptorSet > m_descriptor_sets;

    VkPipelineLayout m_pipeline_layout;
    VkPipeline m_pipeline;

    float m_rotation_y = 0.0f;
    float m_rotation_x = 0.0f;
    float m_rotation_z = 0.0f;

    uint32_t m_indices_to_draw = 0;

    struct
    {
        VkDeviceMemory memory;
        VkBuffer buffer;
    } m_vertices;

    struct
    {
        VkDeviceMemory memory;
        VkBuffer buffer;
    } m_indices;

    struct
    {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView image_view;
        VkSampler image_sampler;
    } m_texture;

    struct memory_buffer
    {
        VkDeviceMemory memory;
        VkBuffer buffer;
    };

    struct uniform_buffer
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    std::vector< memory_buffer > m_uniform_buffers;

    vulkan_data< application_data::stack_alloc_t >& m_vulkan_data;
};
