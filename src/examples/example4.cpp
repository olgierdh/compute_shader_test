#include "examples/example4.hpp"

#include "debug.hpp"
#include "logger.hpp"

#include <cstring>
#include <fstream>
#include <array>

void example4::initialize()
{
    m_vulkan_data.get_memory_budget();

    m_cmd_draw.resize( m_vulkan_data.swap_chain.images_count );
    m_framebuffers.resize( m_vulkan_data.swap_chain.images_count );

    init_render_pass();
    init_framebuffers_and_images();

    init_descriptor_set_layout();
    create_uniform_buffers();
    create_descriptor_pool();

    const auto the_model = load_model( "media/cat.obj" );

    create_vertex_buffer( the_model.vertex_data );
    create_index_buffer( the_model.index_data );

    const auto the_image = load_image( "media/cat.png" );

    create_texture( the_image );

    create_descriptor_sets();
    init_pipeline();

    m_indices_to_draw = the_model.index_data.size();

    init_command_buffer();

    m_vulkan_data.get_memory_budget();
}

void example4::deinitialize()
{
    vkDeviceWaitIdle( m_vulkan_data.logical_device );

    destroy_texture();
    destroy_vertex_buffer();
    destroy_index_buffer();
    destroy_command_buffer();
    destroy_pipeline();
    destroy_descriptor_set_layout();
    destroy_uniform_buffers();
    destroy_descriptor_sets();
    destroy_descriptor_pool();
    destroy_framebuffers_and_images();
    destroy_render_pass();
}

void example4::step( float delta_time_ms )
{
    uint32_t image_idx = 0u;

    vkAcquireNextImageKHR(
        m_vulkan_data.logical_device, m_vulkan_data.swap_chain.swap_chain, UINT64_MAX,
        m_vulkan_data.swap_chain.image_available_semaphore, nullptr, &image_idx );

    update_unform_buffer( delta_time_ms, image_idx );

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

void example4::init_render_pass()
{
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
        VK_ATTACHMENT_STORE_OP_STORE,
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
}

void example4::destroy_command_buffer()
{
    vkFreeCommandBuffers( m_vulkan_data.logical_device,
                          m_vulkan_data.pool_command_buffers, m_cmd_draw.size(),
                          m_cmd_draw.data() );
    m_cmd_draw.clear();
}

void example4::destroy_pipeline()
{
    vkDestroyPipeline( m_vulkan_data.logical_device, m_pipeline, nullptr );
    vkDestroyPipelineLayout( m_vulkan_data.logical_device, m_pipeline_layout, nullptr );
}

void example4::init_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding ubo_layout_binding{
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};

    VkDescriptorSetLayoutBinding sampler_layout_binding{
        1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr};

    std::array< VkDescriptorSetLayoutBinding, 2 > bindings = {ubo_layout_binding,
                                                              sampler_layout_binding};

    VkDescriptorSetLayoutCreateInfo create_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 2,
        bindings.data()};

    const auto res = vkCreateDescriptorSetLayout(
        m_vulkan_data.logical_device, &create_info, nullptr, &m_descriptor_set_layout );

    NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Couldn't create descriptor set layout" );
}

void example4::destroy_descriptor_set_layout()
{
    vkDestroyDescriptorSetLayout( m_vulkan_data.logical_device, m_descriptor_set_layout,
                                  nullptr );
}

void example4::destroy_framebuffers_and_images()
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

void example4::destroy_render_pass()
{
    vkDestroyRenderPass( m_vulkan_data.logical_device, m_render_pass, nullptr );
}

void example4::record_command_buffers()
{
    const VkCommandBufferBeginInfo begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr};

    const VkClearColorValue color                = {{0.0f, 0.1f, 0.0f, 1.0f}};
    const VkClearDepthStencilValue depth_stencil = {1.0f, 0};
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
        vkCmdBindVertexBuffers( m_cmd_draw[idx], 0, 1, &m_vertices.buffer, &offsets );
        vkCmdBindIndexBuffer( m_cmd_draw[idx], m_indices.buffer, 0,
                              VK_INDEX_TYPE_UINT32 );

        vkCmdBindDescriptorSets( m_cmd_draw[idx], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 m_pipeline_layout, 0, 1, &m_descriptor_sets[idx], 0,
                                 nullptr );

        vkCmdDrawIndexed( m_cmd_draw[idx], m_indices_to_draw, 1, 0, 0, 0 );

        vkCmdEndRenderPass( m_cmd_draw[idx] );

        vkEndCommandBuffer( m_cmd_draw[idx] );
    }
}

void example4::init_framebuffers_and_images()
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

void example4::init_command_buffer()
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

void example4::create_vertex_buffer( const std::vector< vtx_t::vertex >& vertices )
{
    uint32_t vertex_buffer_size =
        static_cast< uint32_t >( vertices.size() ) * sizeof( vtx_t::vertex );

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
                               &m_vertices.buffer );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Creating buffer memory failed" );

    vkGetBufferMemoryRequirements( m_vulkan_data.logical_device, m_vertices.buffer,
                                   &mem_requirements );

    const auto memory_type_idx = m_vulkan_data.get_memory_type_idx(
        mem_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    VkMemoryAllocateInfo mem_alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                      mem_requirements.size, memory_type_idx};

    res = vkAllocateMemory( m_vulkan_data.logical_device, &mem_alloc, nullptr,
                            &m_vertices.memory );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Allocating buffer memory failed" );

    void* data;
    res = vkMapMemory( m_vulkan_data.logical_device, m_vertices.memory, 0,
                       mem_alloc.allocationSize, 0, &data );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Mapping buffer memory failed" );

    memcpy( data, vertices.data(), vertex_buffer_size );
    vkUnmapMemory( m_vulkan_data.logical_device, m_vertices.memory );

    res = vkBindBufferMemory( m_vulkan_data.logical_device, m_vertices.buffer,
                              m_vertices.memory, 0 );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Binding buffer memory failed" );
}

void example4::destroy_vertex_buffer()
{
    vkFreeMemory( m_vulkan_data.logical_device, m_vertices.memory, nullptr );
    vkDestroyBuffer( m_vulkan_data.logical_device, m_vertices.buffer, nullptr );
}

void example4::create_index_buffer( const std::vector< vtx_t::index >& indices )
{
    uint32_t index_buffer_size =
        static_cast< uint32_t >( indices.size() ) * sizeof( vtx_t::index );

    VkMemoryRequirements mem_requirements;

    // Vertex buffer
    VkBufferCreateInfo vertex_buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                             nullptr,
                                             0,
                                             index_buffer_size,
                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                             VK_SHARING_MODE_EXCLUSIVE,
                                             0,
                                             nullptr};

    // Copy vertex data to a buffer visible to the host
    auto res = vkCreateBuffer( m_vulkan_data.logical_device, &vertex_buffer_info, nullptr,
                               &m_indices.buffer );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Creating buffer memory failed" );

    vkGetBufferMemoryRequirements( m_vulkan_data.logical_device, m_indices.buffer,
                                   &mem_requirements );

    const auto memory_type_idx = m_vulkan_data.get_memory_type_idx(
        mem_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    VkMemoryAllocateInfo mem_alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                      mem_requirements.size, memory_type_idx};

    res = vkAllocateMemory( m_vulkan_data.logical_device, &mem_alloc, nullptr,
                            &m_indices.memory );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Allocating buffer memory failed" );

    void* data;
    res = vkMapMemory( m_vulkan_data.logical_device, m_indices.memory, 0,
                       mem_alloc.allocationSize, 0, &data );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Mapping buffer memory failed" );

    memcpy( data, indices.data(), index_buffer_size );
    vkUnmapMemory( m_vulkan_data.logical_device, m_indices.memory );

    res = vkBindBufferMemory( m_vulkan_data.logical_device, m_indices.buffer,
                              m_indices.memory, 0 );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Binding buffer memory failed" );
}

void example4::destroy_index_buffer()
{
    vkFreeMemory( m_vulkan_data.logical_device, m_indices.memory, nullptr );
    vkDestroyBuffer( m_vulkan_data.logical_device, m_indices.buffer, nullptr );
}

void example4::init_pipeline()
{
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
        VK_TRUE,
        VK_TRUE,
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

    VkVertexInputBindingDescription vertex_input_binding = {0, sizeof( vtx_t::vertex ),
                                                            VK_VERTEX_INPUT_RATE_VERTEX};

    std::array< VkVertexInputAttributeDescription, 3 > attributes = {
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                          offsetof( vtx_t::vertex, position )},
        VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                          offsetof( vtx_t::vertex, normal )},
        VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32_SFLOAT,
                                          offsetof( vtx_t::vertex, texcoord )}};

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr,
        0,
        1,
        &vertex_input_binding,
        attributes.size(),
        attributes.data()};

    VkPipelineShaderStageCreateInfo shader_stage_vertex = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        0,
        VK_SHADER_STAGE_VERTEX_BIT,
        m_vulkan_data.load_shader( "generated/example4.vert.spirv" ),
        "main",
        nullptr};

    VkPipelineShaderStageCreateInfo shader_stage_fragment = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        0,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        m_vulkan_data.load_shader( "generated/example4.frag.spirv" ),
        "main",
        nullptr};

    std::array< VkPipelineShaderStageCreateInfo, 2 > shader_stages{shader_stage_vertex,
                                                                   shader_stage_fragment};

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        1,
        &m_descriptor_set_layout,
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

    vkDestroyShaderModule( m_vulkan_data.logical_device, shader_stages[0].module,
                           nullptr );
    vkDestroyShaderModule( m_vulkan_data.logical_device, shader_stages[1].module,
                           nullptr );
}

void example4::create_uniform_buffers()
{
    VkDeviceSize ubo_size = sizeof( uniform_buffer );

    m_uniform_buffers.resize( m_vulkan_data.swap_chain.images_count );

    for ( auto& ub : m_uniform_buffers )
    {
#define FOR_STUDENTS_BEGIN
        VkMemoryRequirements memory_requirements{};

        VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                          nullptr,
                                          0,
                                          ubo_size,
                                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                          VK_SHARING_MODE_EXCLUSIVE,
                                          0,
                                          nullptr};

        // Copy vertex data to a buffer visible to the host
        auto res = vkCreateBuffer( m_vulkan_data.logical_device, &create_info, nullptr,
                                   &ub.buffer );
        NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Creating buffer memory failed" );

        vkGetBufferMemoryRequirements( m_vulkan_data.logical_device, ub.buffer,
                                       &memory_requirements );

        const auto memory_type_idx = m_vulkan_data.get_memory_type_idx(
            memory_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

        VkMemoryAllocateInfo mem_alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                          memory_requirements.size, memory_type_idx};

        res = vkAllocateMemory( m_vulkan_data.logical_device, &mem_alloc, nullptr,
                                &ub.memory );
        NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Allocating buffer memory failed" );


        res = vkBindBufferMemory( m_vulkan_data.logical_device, ub.buffer, ub.memory, 0 );
        NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Binding buffer memory failed" );
#define FOR_STUDENTS_END
    }
}

void example4::destroy_uniform_buffers()
{
    for ( auto& ub : m_uniform_buffers )
    {
        vkDestroyBuffer( m_vulkan_data.logical_device, ub.buffer, nullptr );
        vkFreeMemory( m_vulkan_data.logical_device, ub.memory, nullptr );
    }
}

void example4::create_descriptor_pool()
{
    std::array< VkDescriptorPoolSize, 2 > pool_size;
    pool_size[0] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    m_vulkan_data.swap_chain.images_count};
    pool_size[1] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    m_vulkan_data.swap_chain.images_count};

    VkDescriptorPoolCreateInfo create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr, 0,
        m_vulkan_data.swap_chain.images_count,         2,       pool_size.data()};

    const auto res = vkCreateDescriptorPool( m_vulkan_data.logical_device, &create_info,
                                             nullptr, &m_descriptor_pool );
    NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Couldn't create descriptor pool!" );
}

void example4::destroy_descriptor_pool()
{
    vkDestroyDescriptorPool( m_vulkan_data.logical_device, m_descriptor_pool, nullptr );
}


void example4::create_descriptor_sets()
{
    std::vector< VkDescriptorSetLayout > layouts( m_vulkan_data.swap_chain.images_count,
                                                  m_descriptor_set_layout );
    VkDescriptorSetAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, m_descriptor_pool,
        static_cast< uint32_t >( layouts.size() ), layouts.data()};

    m_descriptor_sets.resize( m_vulkan_data.swap_chain.images_count );

    const auto res = vkAllocateDescriptorSets( m_vulkan_data.logical_device, &allocInfo,
                                               m_descriptor_sets.data() );

    NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Couldn't allocate descriptor sets!" );

    for ( auto i = 0u; i < m_vulkan_data.swap_chain.images_count; ++i )
    {
#define FOR_STUDENTS_BEGIN
        VkDescriptorBufferInfo buffer_info{m_uniform_buffers[i].buffer, 0,
                                           sizeof( uniform_buffer )};

        VkDescriptorImageInfo image_info{m_texture.image_sampler, m_texture.image_view,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        std::array< VkWriteDescriptorSet, 2 > descriptor_writes{};

        descriptor_writes[0] =
            VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                 nullptr,
                                 m_descriptor_sets[i],
                                 0,
                                 0,
                                 1,
                                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                 nullptr,
                                 &buffer_info,
                                 nullptr};

        descriptor_writes[1] =
            VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                 nullptr,
                                 m_descriptor_sets[i],
                                 1,
                                 0,
                                 1,
                                 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 &image_info,
                                 nullptr,
                                 nullptr};

        vkUpdateDescriptorSets( m_vulkan_data.logical_device, 2, descriptor_writes.data(),
                                0, nullptr );
#define FOR_STUDENTS_END
    }
}

void example4::destroy_descriptor_sets()
{
    // We don't need to destroy descriptor sets as it will dissapear after the release of
    // the pool
}

void example4::update_unform_buffer( float dt_s, uint32_t current_image )
{
    uniform_buffer ubo{};

    m_rotation_y += dt_s * 0.75;
    // m_rotation_x += dt_s * 1.25;
    // m_rotation_z += dt_s * 0.25;

    ubo.model = glm::rotate( glm::mat4( 1.0f ), m_rotation_x * glm::radians( 90.0f ),
                             glm::vec3( 1.0f, 0.0f, 0.0f ) );

    ubo.model *= glm::rotate( glm::mat4( 1.0f ), m_rotation_y * glm::radians( 90.0f ),
                              glm::vec3( 0.0f, 1.0f, 0.0f ) );

    ubo.model *= glm::rotate( glm::mat4( 1.0f ), m_rotation_z * glm::radians( 90.0f ),
                              glm::vec3( 0.0f, 0.0f, 1.0f ) );

    ubo.view = glm::lookAt( glm::vec3( 0.0f, 0.0f, 50.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ),
                            glm::vec3( 0.0f, 1.0f, 0.0f ) );

    ubo.proj = glm::perspective( glm::radians( 80.0f ),
                                 WIDTH / static_cast< float >( HEIGHT ), 0.1f, 100.0f );

    void* data     = nullptr;
    const auto res = vkMapMemory( m_vulkan_data.logical_device,
                                  m_uniform_buffers[current_image].memory, 0,
                                  sizeof( uniform_buffer ), 0, &data );
    NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Couldn't map uniform buffer memory!" );
    memcpy( data, &ubo, sizeof( uniform_buffer ) );
    vkUnmapMemory( m_vulkan_data.logical_device,
                   m_uniform_buffers[current_image].memory );
}

void example4::create_texture( const image& img )
{
    VkImageCreateInfo image_create_info{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,
        VK_IMAGE_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        VkExtent3D{static_cast< uint32_t >( img.width ),
                   static_cast< uint32_t >( img.height ), 1},
        1,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED};

    {
        const auto res = vkCreateImage( m_vulkan_data.logical_device, &image_create_info,
                                        nullptr, &m_texture.image );
        NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Couldn't create image!" );
    }

    VkMemoryRequirements memory_req{};
    vkGetImageMemoryRequirements( m_vulkan_data.logical_device, m_texture.image,
                                  &memory_req );

    VkMemoryAllocateInfo allocate_info{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memory_req.size,
        m_vulkan_data.get_memory_type_idx( memory_req.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )};

    {
        const auto res = vkAllocateMemory( m_vulkan_data.logical_device, &allocate_info,
                                           nullptr, &m_texture.memory );
        NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Can't allocate memory for an image!" );
    }

    vkBindImageMemory( m_vulkan_data.logical_device, m_texture.image, m_texture.memory,
                       0 );


    copy_texture_data( img );

    // create image view
    {
        VkImageViewCreateInfo create_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                          nullptr,
                                          0,
                                          m_texture.image,
                                          VK_IMAGE_VIEW_TYPE_2D,
                                          VK_FORMAT_R8G8B8A8_UNORM,
                                          {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                           VK_COMPONENT_SWIZZLE_B,
                                           VK_COMPONENT_SWIZZLE_A},
                                          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

        {
            const auto res =
                vkCreateImageView( m_vulkan_data.logical_device, &create_info, nullptr,
                                   &m_texture.image_view );
            NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Couldn't create image view!" );
        }
    }

    {
        VkSamplerCreateInfo create_info{
            VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            nullptr,
            0,
            VK_FILTER_LINEAR,
            VK_FILTER_LINEAR,
            VK_SAMPLER_MIPMAP_MODE_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            0,
            VK_FALSE,
            0.0f,
            VK_FALSE,
            VK_COMPARE_OP_NEVER,
            0.0,
            0.0,
            VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
            VK_FALSE,
        };

        const auto res = vkCreateSampler( m_vulkan_data.logical_device, &create_info,
                                          nullptr, &m_texture.image_sampler );
        NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Couldn't create image sampler!" );
    }
}

void example4::copy_texture_data( const image& img )
{
    VkBuffer staging_buffer              = nullptr;
    VkDeviceMemory staging_buffer_memory = nullptr;

    const auto image_data_size = img.width * img.height * img.channels;

    VkBufferCreateInfo staging_buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                              nullptr,
                                              0,
                                              static_cast< uint32_t >( image_data_size ),
                                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VK_SHARING_MODE_EXCLUSIVE,
                                              0,
                                              nullptr};

    // Copy vertex data to a buffer visible to the host
    auto res = vkCreateBuffer( m_vulkan_data.logical_device, &staging_buffer_info,
                               nullptr, &staging_buffer );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Creating buffer memory failed" );

    VkMemoryRequirements memory_req{};
    vkGetBufferMemoryRequirements( m_vulkan_data.logical_device, staging_buffer,
                                   &memory_req );

    const auto memory_type_idx = m_vulkan_data.get_memory_type_idx(
        memory_req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    VkMemoryAllocateInfo mem_alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                      memory_req.size, memory_type_idx};

    res = vkAllocateMemory( m_vulkan_data.logical_device, &mem_alloc, nullptr,
                            &staging_buffer_memory );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Allocating buffer memory failed" );

    void* data;
    res = vkMapMemory( m_vulkan_data.logical_device, staging_buffer_memory, 0,
                       mem_alloc.allocationSize, 0, &data );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Mapping buffer memory failed" );

    memcpy( data, img.data.data(), image_data_size );
    vkUnmapMemory( m_vulkan_data.logical_device, staging_buffer_memory );

    res = vkBindBufferMemory( m_vulkan_data.logical_device, staging_buffer,
                              staging_buffer_memory, 0 );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Binding buffer memory failed" );

    ///////////////////////////////////////////////////////////////////////////////////////////

    VkCommandBufferAllocateInfo allocate_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
        m_vulkan_data.pool_command_buffers, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};

    VkCommandBuffer command_buffer = nullptr;

    {
        const auto res = vkAllocateCommandBuffers( m_vulkan_data.logical_device,
                                                   &allocate_info, &command_buffer );
        NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Can't alocate command buffer!" );
    }

    VkCommandBufferBeginInfo begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr};

    {
        const auto res = vkBeginCommandBuffer( command_buffer, &begin_info );
        NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Can't begin command buffer!" );
    }

    // transfer image layout from undefinded -> dst_optimal

    ////////////////////////////////////////////////////////////////////////////////////////

    VkImageMemoryBarrier barrier_from_undefined_to_dst_optimal = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        m_texture.image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    vkCmdPipelineBarrier( command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                          &barrier_from_undefined_to_dst_optimal );
    // copy data
    VkBufferImageCopy region = {
        0,
        static_cast< uint32_t >( img.width ),
        static_cast< uint32_t >( img.height ),
        VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        VkOffset3D{0, 0, 0},
        VkExtent3D{static_cast< uint32_t >( img.width ),
                   static_cast< uint32_t >( img.height ), static_cast< uint32_t >( 1 )}};

    vkCmdCopyBufferToImage( command_buffer, staging_buffer, m_texture.image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

    // transfer image layout dst_optimal -> shader_read_optimal

    VkImageMemoryBarrier barrier_from_dst_optimal_to_shader_read = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        m_texture.image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    vkCmdPipelineBarrier( command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                          nullptr, 1, &barrier_from_dst_optimal_to_shader_read );

    vkEndCommandBuffer( command_buffer );

    const VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                      nullptr,
                                      0,
                                      nullptr,
                                      nullptr,
                                      1,
                                      &command_buffer,
                                      0,
                                      nullptr};

    // execute cmd buffer
    {
        const auto res =
            vkQueueSubmit( m_vulkan_data.graphics_queue, 1, &submit_info, nullptr );
        NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Submission of buffer failed!" );
    }

    {
        const auto res = vkQueueWaitIdle( m_vulkan_data.graphics_queue );
        NEO_ASSERT_ALWAYS( res == VK_SUCCESS, "Submission of buffer failed!" );
    }

    // delete staging buffer
    vkFreeMemory( m_vulkan_data.logical_device, staging_buffer_memory, nullptr );
    vkDestroyBuffer( m_vulkan_data.logical_device, staging_buffer, nullptr );
}

void example4::destroy_texture()
{
    vkFreeMemory( m_vulkan_data.logical_device, m_texture.memory, nullptr );
    vkDestroyImage( m_vulkan_data.logical_device, m_texture.image, nullptr );
    vkDestroyImageView( m_vulkan_data.logical_device, m_texture.image_view, nullptr );
    vkDestroySampler( m_vulkan_data.logical_device, m_texture.image_sampler, nullptr );
}
