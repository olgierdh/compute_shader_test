#include "examples/example1.hpp"

#include "debug.hpp"
#include "logger.hpp"

void example1::initialize()
{
    m_cmd_clear_screen.resize( m_vulkan_data.swap_chain.images_count );
    init_command_buffer_pool();
    init_command_buffer();
}

void example1::step( float /*delta_time_ms*/ )
{
    uint32_t image_idx = 0u;

#define FOR_STUDENTS_BEGIN
    auto res = vkAcquireNextImageKHR(
        m_vulkan_data.logical_device, m_vulkan_data.swap_chain.swap_chain, UINT64_MAX,
        m_vulkan_data.swap_chain.image_available_semaphore, nullptr, &image_idx );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Next image acquire failed" );

    const VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    const VkSubmitInfo submit_info        = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        1,
        &m_vulkan_data.swap_chain.image_available_semaphore,
        &stage_mask,
        1,
        &m_cmd_clear_screen[image_idx],
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

    res = vkQueuePresentKHR( m_vulkan_data.graphics_queue, &present_info );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Queue presentation failed" );
#define FOR_STUDENTS_END
}

void example1::deinitialize()
{
    vkDeviceWaitIdle( m_vulkan_data.logical_device );

    vkDestroySemaphore( m_vulkan_data.logical_device,
                        m_vulkan_data.swap_chain.image_available_semaphore, nullptr );

    vkDestroySemaphore( m_vulkan_data.logical_device,
                        m_vulkan_data.swap_chain.rendering_finished_semaphore, nullptr );

    vkDestroyCommandPool( m_vulkan_data.logical_device,
                          m_vulkan_data.pool_command_buffers, nullptr );
}

void example1::record_command_buffer()
{
    const VkCommandBufferBeginInfo begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr};

    const VkImageSubresourceRange image_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    const VkClearColorValue color = VkClearColorValue{{0.0, 0.5, 0.0, 1.0}};

    const auto cmd_count = m_cmd_clear_screen.size();
    for ( uint32_t idx = 0; idx < cmd_count; ++idx )
    {
        vkBeginCommandBuffer( m_cmd_clear_screen[idx], &begin_info );

        VkImageMemoryBarrier barrier_presentation_to_clear = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            nullptr,
            VK_ACCESS_MEMORY_READ_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast< uint32_t >( m_vulkan_data.selected_gfx_queue_idx ),
            static_cast< uint32_t >( m_vulkan_data.selected_gfx_queue_idx ),
            m_vulkan_data.swap_chain.swap_chain_images[idx],
            image_range};

        vkCmdPipelineBarrier( m_cmd_clear_screen[idx], VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                              1, &barrier_presentation_to_clear );

        vkCmdClearColorImage(
            m_cmd_clear_screen[idx], m_vulkan_data.swap_chain.swap_chain_images[idx],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &image_range );

        VkImageMemoryBarrier barrier_clear_to_present = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            nullptr,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_MEMORY_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            static_cast< uint32_t >( m_vulkan_data.selected_gfx_queue_idx ),
            static_cast< uint32_t >( m_vulkan_data.selected_gfx_queue_idx ),
            m_vulkan_data.swap_chain.swap_chain_images[idx],
            image_range};

        vkCmdPipelineBarrier( m_cmd_clear_screen[idx], VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                              nullptr, 1, &barrier_clear_to_present );

        vkEndCommandBuffer( m_cmd_clear_screen[idx] );
    }
}

void example1::init_command_buffer()
{
    const VkCommandBufferAllocateInfo cmd_alloc_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
        m_vulkan_data.pool_command_buffers, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        m_vulkan_data.swap_chain.images_count};

    const VkResult res = vkAllocateCommandBuffers(
        m_vulkan_data.logical_device, &cmd_alloc_info, m_cmd_clear_screen.data() );

    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Command buffer allocation failed" );

    record_command_buffer();
}

void example1::init_command_buffer_pool()
{
    const VkCommandPoolCreateInfo cmd_pool_create_info = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        static_cast< uint32_t >( m_vulkan_data.selected_gfx_queue_idx )};


    const VkResult res =
        vkCreateCommandPool( m_vulkan_data.logical_device, &cmd_pool_create_info, nullptr,
                             &m_vulkan_data.pool_command_buffers );
    NEO_ASSERT_ALWAYS( VK_SUCCESS == res, "Command pool creation failed" );
}
