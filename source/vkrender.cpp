#include "vkrender.h"
#include <iostream>
#include <vector>
#include <array>

bool VkRenderer::init(GLFWwindow* win) {
    window = win;

    // 1. Create Vulkan Instance
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Abyss Engine Hub";
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    create_info.enabledExtensionCount = glfw_ext_count;
    create_info.ppEnabledExtensionNames = glfw_exts;

    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) return false;

    // 2. Create Surface
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) return false;

    // 3. Pick Physical Device
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    physical_device = devices[0];

    // 4. Find Queue Families (Graphics & Present)
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
    
    uint32_t graphics_family = 0;
    uint32_t present_family = 0;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphics_family = i;
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        if (present_support) present_family = i;
        if (graphics_family != -1 && present_family != -1) break;
    }

    // 5. Create Logical Device
    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::vector<uint32_t> unique_families = { graphics_family, present_family };
    
    for (uint32_t family : unique_families) {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};
    const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = (uint32_t)queue_create_infos.size();
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = device_extensions;

    if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS) return false;
    vkGetDeviceQueue(device, graphics_family, 0, &graphics_queue);
    vkGetDeviceQueue(device, present_family, 0, &present_queue);

    // 6. Create Swapchain
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());
    VkSurfaceFormatKHR surface_format = formats[0];

    VkSwapchainCreateInfoKHR swap_create_info{};
    swap_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_create_info.surface = surface;
    swap_create_info.minImageCount = capabilities.minImageCount + 1;
    swap_create_info.imageFormat = surface_format.format;
    swap_create_info.imageColorSpace = surface_format.colorSpace;
    swap_create_info.imageExtent = capabilities.currentExtent;
    swap_create_info.imageArrayLayers = 1;
    swap_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_create_info.preTransform = capabilities.currentTransform;
    swap_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    if (vkCreateSwapchainKHR(device, &swap_create_info, nullptr, &swapchain) != VK_SUCCESS) return false;

    uint32_t image_count;
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());

    // 7. Create Image Views
    swapchain_image_views.resize(swapchain_images.size());
    for (size_t i = 0; i < swapchain_images.size(); i++) {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = swapchain_images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = surface_format.format;
        view_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCreateImageView(device, &view_info, nullptr, &swapchain_image_views[i]);
    }

    // 8. Create Render Pass
    VkAttachmentDescription color_attachment{};
    color_attachment.format = surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref{};
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass);

    // 9. Create Framebuffers
    swapchain_framebuffers.resize(swapchain_image_views.size());
    for (size_t i = 0; i < swapchain_image_views.size(); i++) {
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = &swapchain_image_views[i];
        framebuffer_info.width = capabilities.currentExtent.width;
        framebuffer_info.height = capabilities.currentExtent.height;
        framebuffer_info.layers = 1;
        vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swapchain_framebuffers[i]);
    }

    // 10. Create Command Pool & Buffers
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = graphics_family;
    vkCreateCommandPool(device, &pool_info, nullptr, &command_pool);

    command_buffers.resize(swapchain_framebuffers.size());
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = (uint32_t)command_buffers.size();
    vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data());

    // 11. Create Sync Objects
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphore);
    vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphore);
    vkCreateFence(device, &fence_info, nullptr, &in_flight_fence);

    // 12. Create Descriptor Pool for ImGui
    VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 } };
    VkDescriptorPoolCreateInfo dp_info{};
    dp_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dp_info.maxSets = 1000;
    dp_info.poolSizeCount = 1;
    dp_info.pPoolSizes = pool_sizes;
    vkCreateDescriptorPool(device, &dp_info, nullptr, &descriptor_pool);

    // 13. Init ImGui Vulkan Backend
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physical_device;
    init_info.Device = device;
    init_info.QueueFamily = graphics_family;
    init_info.Queue = graphics_queue;
    init_info.DescriptorPool = descriptor_pool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = (uint32_t)swapchain_images.size();
    
    ImGui_ImplVulkan_Init(&init_info, render_pass);
    
    // Upload Fonts
    VkCommandBuffer cmd = command_buffers[0];
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &begin_info);
    ImGui_ImplVulkan_CreateFontsTexture();
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    return true;
}

void VkRenderer::draw_frame() {
    vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &in_flight_fence);

    uint32_t image_index;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);

    vkResetCommandBuffer(command_buffers[image_index], 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(command_buffers[image_index], &begin_info);

    VkClearValue clear_color = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = swapchain_framebuffers[image_index];
    render_pass_info.renderArea.extent = { 1920, 1080 }; // Simplified for now
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffers[image_index], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    // Draw ImGui
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffers[image_index]);

    vkCmdEndRenderPass(command_buffers[image_index]);
    vkEndCommandBuffer(command_buffers[image_index]);

    // Submit
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = { image_available_semaphore };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[image_index];
    VkSemaphore signal_semaphores[] = { render_finished_semaphore };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence);

    // Present
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(present_queue, &present_info);
}

void VkRenderer::shutdown() {
    vkDeviceWaitIdle(device);
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    vkDestroySemaphore(device, render_finished_semaphore, nullptr);
    vkDestroySemaphore(device, image_available_semaphore, nullptr);
    vkDestroyFence(device, in_flight_fence, nullptr);
    vkDestroyCommandPool(device, command_pool, nullptr);
    for (auto fb : swapchain_framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    for (auto iv : swapchain_image_views) vkDestroyImageView(device, iv, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}