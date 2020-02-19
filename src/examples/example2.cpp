#include "examples/example2.hpp"

#include "debug.hpp"
#include "logger.hpp"

#include <cstring>
#include <fstream>
#include <array>

void example2::initialize()
{
    m_cmd_draw.resize( m_vulkan_data.swap_chain.images_count );
    m_framebuffers.resize( m_vulkan_data.swap_chain.images_count );

    init_render_pass();
    init_framebuffers_and_images();

    init_pipeline();

    prepare_vertices();
    init_command_buffer();
}

void example2::step( float /*delta_time_ms*/ )
{
    uint32_t image_idx = 0u;

    vkAcquireNextImageKHR(
        m_vulkan_data.logical_device, m_vulkan_data.swap_chain.swap_chain, UINT64_MAX,
        m_vulkan_data.swap_chain.image_available_semaphore, nullptr, &image_idx );

    const VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    const VkSubmitInfo submit_info        = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        1,
        &m_vulkan_data.swap_chain.image_available_semaphore,
        &stage_mask,
        1,
        &m_cmd_draw[image_idx],
        1,
        &m_vulkan_data.swap_chain.rendering_finished_semaphore};

    vkQueueSubmit( m_vulkan_data.graphics_queue, 1, &submit_info, nullptr );

    const VkPresentInfoKHR present_info = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1,
        &m_vulkan_data.swap_chain.rendering_finished_semaphore,
        1,
        &m_vulkan_data.swap_chain.swap_chain,
        &image_idx,
        nullptr};

    const VkResult res = vkQueuePresentKHR( m_vulkan_data.graphics_queue, &present_info );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Queue presentation failed" );
}

void example2::deinitialize()
{
    vkDeviceWaitIdle( m_vulkan_data.logical_device );

    destroy_vertices();
    destroy_command_buffer();
    destroy_pipeline();
    destroy_framebuffers_and_images();
    destroy_render_pass();
}

void example2::init_render_pass()
{
#define FOR_STUDENTS_BEGIN
    // Color attachment
    VkAttachmentDescription attachment_color = {
        0,
        m_vulkan_data.swap_chain.selected_format.format,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    VkAttachmentDescription attachment_depth_stencil = {
        0,
        m_vulkan_data.depth_format,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    std::array< VkAttachmentDescription, 2 > attachments = {attachment_color,
                                                            attachment_depth_stencil};

    VkAttachmentReference color_reference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkAttachmentReference depth_stencil_reference = {
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass_description = {
        0,       VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 1, &color_reference,
        nullptr, &depth_stencil_reference,        0, nullptr};

    VkSubpassDependency subpass_final_to_initial = {
        VK_SUBPASS_EXTERNAL,
        0,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_DEPENDENCY_BY_REGION_BIT};

    VkSubpassDependency subpass_initial_to_final = {
        VK_SUBPASS_EXTERNAL,
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_DEPENDENCY_BY_REGION_BIT};

    std::array< VkSubpassDependency, 2 > dependencies = {subpass_final_to_initial,
                                                         subpass_initial_to_final};

    // Create the actual renderpass
    VkRenderPassCreateInfo renderpass_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        static_cast< uint32_t >( attachments.size() ),
        attachments.data(),
        1,
        &subpass_description,
        static_cast< uint32_t >( dependencies.size() ),
        dependencies.data()};

    const auto res = vkCreateRenderPass( m_vulkan_data.logical_device, &renderpass_info,
                                         nullptr, &m_render_pass );

    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Command buffer allocation failed" );
#define FOR_STUDENTS_END
}

void example2::destroy_command_buffer()
{
    vkFreeCommandBuffers( m_vulkan_data.logical_device,
                          m_vulkan_data.pool_command_buffers, m_cmd_draw.size(),
                          m_cmd_draw.data() );
    m_cmd_draw.clear();
}

void example2::destroy_pipeline()
{
    vkDestroyPipeline( m_vulkan_data.logical_device, m_pipeline, nullptr );
    vkDestroyPipelineLayout( m_vulkan_data.logical_device, m_pipeline_layout, nullptr );
}

void example2::destroy_framebuffers_and_images()
{
    for ( auto& fb : m_framebuffers )
    {
        vkDestroyFramebuffer( m_vulkan_data.logical_device, fb, nullptr );
    }

    m_framebuffers.clear();

    vkDestroyImageView( m_vulkan_data.logical_device, m_depth_stencil_image_view,
                        nullptr );

    vkDestroyImage( m_vulkan_data.logical_device, m_depth_stencil_image, nullptr );
    vkFreeMemory( m_vulkan_data.logical_device, m_depth_stencil_memory, nullptr );
}

void example2::destroy_render_pass()
{
    vkDestroyRenderPass( m_vulkan_data.logical_device, m_render_pass, nullptr );
}

void example2::record_command_buffers()
{
    const VkCommandBufferBeginInfo begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr};

    const VkClearColorValue color                = {{0.0f, 0.1f, 0.0f, 1.0f}};
    const VkClearDepthStencilValue depth_stencil = {0.0f, 0};
    VkClearValue clear_values[2]                 = {};
    clear_values[0].color                        = color;
    clear_values[1].depthStencil                 = depth_stencil;

    VkRenderPassBeginInfo render_pass_begin_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        m_render_pass,
        {},
        {{0, 0}, {WIDTH, HEIGHT}},
        2,
        clear_values};

    const auto cmd_count = m_cmd_draw.size();
    for ( uint32_t idx = 0; idx < cmd_count; ++idx )
    {
        render_pass_begin_info.framebuffer = m_framebuffers[idx];

        vkBeginCommandBuffer( m_cmd_draw[idx], &begin_info );

        vkCmdBeginRenderPass( m_cmd_draw[idx], &render_pass_begin_info,
                              VK_SUBPASS_CONTENTS_INLINE );

        vkCmdBindPipeline( m_cmd_draw[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline );

        // Bind triangle vertex buffer (contains position and colors)
        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers( m_cmd_draw[idx], 0, 1, &vertices.buffer, &offsets );

        vkCmdDraw( m_cmd_draw[idx], 3, 1, 0, 0 );

        vkCmdEndRenderPass( m_cmd_draw[idx] );

        vkEndCommandBuffer( m_cmd_draw[idx] );
    }
}

void example2::init_framebuffers_and_images()
{
    const auto swapchain_image_count = m_vulkan_data.swap_chain.images_count;

    // Depth stencil image and depth stencil image view
    VkImageCreateInfo depth_stencil_image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,
        VK_IMAGE_TYPE_2D,
        m_vulkan_data.depth_format,
        {WIDTH, HEIGHT, 1},
        1,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED};

    auto res =
        vkCreateImage( m_vulkan_data.logical_device, &depth_stencil_image_create_info,
                       nullptr, &m_depth_stencil_image );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Failed to create stencil image" );

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements( m_vulkan_data.logical_device, m_depth_stencil_image,
                                  &memory_requirements );

    const auto memory_type_index = m_vulkan_data.get_memory_type_idx(
        memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    VkMemoryAllocateInfo mem_alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                      memory_requirements.size, memory_type_index};

    res = vkAllocateMemory( m_vulkan_data.logical_device, &mem_alloc, nullptr,
                            &m_depth_stencil_memory );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res,
                       "Allocate memory for depth stencil image failed" );

    res = vkBindImageMemory( m_vulkan_data.logical_device, m_depth_stencil_image,
                             m_depth_stencil_memory, 0 );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Bind memory for depth stencil image failed" );

    VkImageViewCreateInfo depth_stencil_image_view_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        m_depth_stencil_image,
        VK_IMAGE_VIEW_TYPE_2D,
        m_vulkan_data.depth_format,
        {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
         VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
        {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1}};

    res = vkCreateImageView( m_vulkan_data.logical_device,
                             &depth_stencil_image_view_create_info, nullptr,
                             &m_depth_stencil_image_view );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res,
                       "Creating image view for depth stencil image failed" );

    // Framebuffers for swapchain color images
    for ( size_t idx = 0; idx < swapchain_image_count; ++idx )
    {
        std::array< VkImageView, 2 > attachments = {
            m_vulkan_data.swap_chain.swap_chain_image_views[idx],
            m_depth_stencil_image_view};

        VkFramebufferCreateInfo framebuffer_create_info = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr,
            0,
            m_render_pass,
            static_cast< uint32_t >( attachments.size() ),
            attachments.data(),
            WIDTH,
            HEIGHT,
            1};

        // Create the framebuffer
        const auto res =
            vkCreateFramebuffer( m_vulkan_data.logical_device, &framebuffer_create_info,
                                 nullptr, &m_framebuffers[idx] );
        NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Framebuffer creation failed" );
    }
}

void example2::init_command_buffer()
{
    const VkCommandBufferAllocateInfo cmd_alloc_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
        m_vulkan_data.pool_command_buffers, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        m_vulkan_data.swap_chain.images_count};

    const auto res = vkAllocateCommandBuffers( m_vulkan_data.logical_device,
                                               &cmd_alloc_info, m_cmd_draw.data() );

    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Command buffer allocation failed" );

    record_command_buffers();
}

void example2::prepare_vertices()
{
    std::vector< vertex > vertex_buffer = {
        vertex{{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        vertex{{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        vertex{{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

    uint32_t vertex_buffer_size =
        static_cast< uint32_t >( vertex_buffer.size() ) * sizeof( vertex );

    VkMemoryRequirements mem_requirements;

    // Vertex buffer
    VkBufferCreateInfo vertex_buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                             nullptr,
                                             0,
                                             vertex_buffer_size,
                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                             VK_SHARING_MODE_EXCLUSIVE,
                                             0,
                                             nullptr};

    // Copy vertex data to a buffer visible to the host
    auto res = vkCreateBuffer( m_vulkan_data.logical_device, &vertex_buffer_info, nullptr,
                               &vertices.buffer );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Creating buffer memory failed" );

    vkGetBufferMemoryRequirements( m_vulkan_data.logical_device, vertices.buffer,
                                   &mem_requirements );

    const auto memory_type_idx = m_vulkan_data.get_memory_type_idx(
        mem_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    VkMemoryAllocateInfo mem_alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                      mem_requirements.size, memory_type_idx};

    res = vkAllocateMemory( m_vulkan_data.logical_device, &mem_alloc, nullptr,
                            &vertices.memory );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Allocating buffer memory failed" );

    void* data;
    res = vkMapMemory( m_vulkan_data.logical_device, vertices.memory, 0,
                       mem_alloc.allocationSize, 0, &data );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Mapping buffer memory failed" );

    memcpy( data, vertex_buffer.data(), vertex_buffer_size );
    vkUnmapMemory( m_vulkan_data.logical_device, vertices.memory );

    res = vkBindBufferMemory( m_vulkan_data.logical_device, vertices.buffer,
                              vertices.memory, 0 );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Binding buffer memory failed" );
}

void example2::destroy_vertices()
{
    vkFreeMemory( m_vulkan_data.logical_device, vertices.memory, nullptr );
    vkDestroyBuffer( m_vulkan_data.logical_device, vertices.buffer, nullptr );
}

void example2::init_pipeline()
{
#define FOR_STUDENTS_BEGIN
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE};

    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_FALSE,
        VK_FALSE,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_NONE,
        VK_FRONT_FACE_COUNTER_CLOCKWISE,
        VK_FALSE,
        0.0f,
        0.0f,
        0.0f,
        1.0f};

    VkPipelineColorBlendAttachmentState blend_attachment_state = {
        VK_FALSE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT};

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_FALSE,
        VK_LOGIC_OP_CLEAR,
        1,
        &blend_attachment_state,
        {0.0f, 0.0f, 0.0f, 0.0f}};

    VkViewport viewport = {};
    viewport.height     = ( float )HEIGHT;
    viewport.width      = ( float )WIDTH;
    viewport.minDepth   = ( float )0.0f;
    viewport.maxDepth   = ( float )1.0f;

    VkRect2D scissor      = {};
    scissor.extent.width  = WIDTH;
    scissor.extent.height = HEIGHT;
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;

    VkPipelineViewportStateCreateInfo viewport_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        nullptr,
        0,
        1,
        &viewport,
        1,
        &scissor};

    VkStencilOpState front_back = {VK_STENCIL_OP_KEEP,
                                   VK_STENCIL_OP_KEEP,
                                   VK_STENCIL_OP_KEEP,
                                   VK_COMPARE_OP_ALWAYS,
                                   0,
                                   0,
                                   0};

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_FALSE,
        VK_FALSE,
        VK_COMPARE_OP_LESS_OR_EQUAL,
        VK_FALSE,
        VK_FALSE,
        front_back,
        front_back,
        0.0f,
        0.0f};

    VkPipelineMultisampleStateCreateInfo multisample_state = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FALSE,
        0.0f,
        nullptr,
        VK_FALSE,
        VK_FALSE};

    VkVertexInputBindingDescription vertex_input_binding = {0, sizeof( vertex ),
                                                            VK_VERTEX_INPUT_RATE_VERTEX};

    std::array< VkVertexInputAttributeDescription, 2 > attributes = {
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                          offsetof( vertex, position )},
        VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                          offsetof( vertex, color )}};

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr,
        0,
        1,
        &vertex_input_binding,
        2,
        attributes.data()};
#define FOR_STUDENTS_END

    VkPipelineShaderStageCreateInfo shader_stage_vertex = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        0,
        VK_SHADER_STAGE_VERTEX_BIT,
        m_vulkan_data.load_shader( "generated/simple.vert.spirv" ),
        "main",
        nullptr};

    VkPipelineShaderStageCreateInfo shader_stage_fragment = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        0,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        m_vulkan_data.load_shader( "generated/simple.frag.spirv" ),
        "main",
        nullptr};

#define FOR_STUDENTS_BEGIN
    std::array< VkPipelineShaderStageCreateInfo, 2 > shader_stages{shader_stage_vertex,
                                                                   shader_stage_fragment};

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        0,
        nullptr,
        0,
        nullptr};

    auto res = vkCreatePipelineLayout( m_vulkan_data.logical_device,
                                       &pipeline_layout_create_info, nullptr,
                                       &m_pipeline_layout );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Creation of pipeline layout failed" );

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        nullptr,
        0,
        shader_stages.size(),
        shader_stages.data(),
        &vertex_input_state,
        &input_assembly_state,
        nullptr,
        &viewport_state,
        &rasterization_state,
        &multisample_state,
        &depth_stencil_state,
        &color_blend_state,
        nullptr,
        m_pipeline_layout,
        m_render_pass,
        0,
        m_pipeline,
        0};


    res = vkCreateGraphicsPipelines( m_vulkan_data.logical_device, nullptr, 1,
                                     &pipeline_create_info, nullptr, &m_pipeline );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Creation of pipeline failed. Go home." );

#define FOR_STUDENTS_END

    vkDestroyShaderModule( m_vulkan_data.logical_device, shader_stage_vertex.module,
                           nullptr );
    vkDestroyShaderModule( m_vulkan_data.logical_device, shader_stage_fragment.module,
                           nullptr );
}
