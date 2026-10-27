#include "vkrender.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <array>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <fstream>

// --- NUKLEAR SETUP ---
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define NK_IMPLEMENTATION
#include "nuklear.h"

#define NK_GLFW_VULKAN_IMPLEMENTATION
#include "nuklear_glfw_vulkan.h"

struct VkRendererImpl {
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    GLFWwindow* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    std::vector<VkImage> overlayImages;
    std::vector<VkDeviceMemory> overlayImageMemories;
    std::vector<VkImageView> overlayImageViews;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    // Overlay Pipeline Resources
    VkSampler overlaySampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout overlayDescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout overlayPipelineLayout = VK_NULL_HANDLE;
    VkPipeline overlayGraphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> overlayDescriptorSets;

    struct nk_context* ctx = nullptr;
    std::chrono::high_resolution_clock::time_point startTime;

    void createInstance() {
        VkApplicationInfo app{};
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pApplicationName = "DATS";
        app.apiVersion = VK_API_VERSION_1_0;

        uint32_t extCount = 0;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&extCount);

        VkInstanceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.pApplicationInfo = &app;
        ci.enabledExtensionCount = extCount;
        ci.ppEnabledExtensionNames = glfwExts;

        if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS)
            throw std::runtime_error("failed to create instance");
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
            throw std::runtime_error("failed to create surface");
    }

    struct QueueFamilies {
        uint32_t graphics = UINT32_MAX;
        uint32_t present = UINT32_MAX;
        bool complete() const { return graphics != UINT32_MAX && present != UINT32_MAX; }
    };

    QueueFamilies findQueues(VkPhysicalDevice dev) {
        QueueFamilies q;
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
        std::vector<VkQueueFamilyProperties> fams(count);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, fams.data());
        for (uint32_t i = 0; i < count; ++i) {
            if (fams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) q.graphics = i;
            VkBool32 present = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present);
            if (present) q.present = i;
            if (q.complete()) break;
        }
        return q;
    }

    void pickDevice() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        if (count == 0) throw std::runtime_error("no Vulkan GPU");
        std::vector<VkPhysicalDevice> devs(count);
        vkEnumeratePhysicalDevices(instance, &count, devs.data());
        for (auto d : devs) {
            if (findQueues(d).complete()) { physicalDevice = d; break; }
        }
        if (!physicalDevice) throw std::runtime_error("no suitable GPU");
    }

    void createDevice() {
        auto q = findQueues(physicalDevice);
        float prio = 1.0f;
        VkDeviceQueueCreateInfo qci{};
        qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qci.queueFamilyIndex = q.graphics;
        qci.queueCount = 1;
        qci.pQueuePriorities = &prio;

        const char* exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        VkDeviceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        ci.queueCreateInfoCount = 1;
        ci.pQueueCreateInfos = &qci;
        ci.enabledExtensionCount = 1;
        ci.ppEnabledExtensionNames = exts;

        if (vkCreateDevice(physicalDevice, &ci, nullptr, &device) != VK_SUCCESS)
            throw std::runtime_error("failed to create device");

        vkGetDeviceQueue(device, q.graphics, 0, &graphicsQueue);
        vkGetDeviceQueue(device, q.present, 0, &presentQueue);
    }

    void createSwapchain() {
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        VkSurfaceFormatKHR fmt = formats[0];

        VkExtent2D extent = caps.currentExtent;
        if (extent.width == UINT32_MAX) {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            extent.width = std::clamp((uint32_t)w, caps.minImageExtent.width, caps.maxImageExtent.width);
            extent.height = std::clamp((uint32_t)h, caps.minImageExtent.height, caps.maxImageExtent.height);
        }

        uint32_t imageCount = caps.minImageCount + 1;

        VkSwapchainCreateInfoKHR ci{};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface = surface;
        ci.minImageCount = imageCount;
        ci.imageFormat = fmt.format;
        ci.imageColorSpace = fmt.colorSpace;
        ci.imageExtent = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ci.preTransform = caps.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;

        if (vkCreateSwapchainKHR(device, &ci, nullptr, &swapChain) != VK_SUCCESS)
            throw std::runtime_error("failed to create swapchain");

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        swapChainImageFormat = fmt.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); ++i) {
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.image = swapChainImages[i];
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = swapChainImageFormat;
            ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            vkCreateImageView(device, &ci, nullptr, &swapChainImageViews[i]);
        }
    }

    void createOverlayImages() {
        overlayImages.resize(swapChainImages.size());
        overlayImageMemories.resize(swapChainImages.size());
        overlayImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); ++i) {
            VkImageCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ci.imageType = VK_IMAGE_TYPE_2D;
            ci.extent = { swapChainExtent.width, swapChainExtent.height, 1 };
            ci.mipLevels = 1;
            ci.arrayLayers = 1;
            ci.format = swapChainImageFormat;
            ci.tiling = VK_IMAGE_TILING_OPTIMAL;
            ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            ci.samples = VK_SAMPLE_COUNT_1_BIT;
            ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(device, &ci, nullptr, &overlayImages[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create overlay image");

            VkMemoryRequirements req;
            vkGetImageMemoryRequirements(device, overlayImages[i], &req);
            VkMemoryAllocateInfo ai{};
            ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize = req.size;
            
            VkPhysicalDeviceMemoryProperties memProps;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
            uint32_t typeIndex = UINT32_MAX;
            for (uint32_t j = 0; j < memProps.memoryTypeCount; ++j) {
                if ((req.memoryTypeBits & (1 << j)) && (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                    typeIndex = j;
                    break;
                }
            }
            ai.memoryTypeIndex = typeIndex;
            if (vkAllocateMemory(device, &ai, nullptr, &overlayImageMemories[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to allocate overlay image memory");
            vkBindImageMemory(device, overlayImages[i], overlayImageMemories[i], 0);

            VkImageViewCreateInfo vi{};
            vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            vi.image = overlayImages[i];
            vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vi.format = swapChainImageFormat;
            vi.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            if (vkCreateImageView(device, &vi, nullptr, &overlayImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create overlay image view");
        }
    }

    void createRenderPass() {
        VkAttachmentDescription color{};
        color.format = swapChainImageFormat;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{};
        sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.colorAttachmentCount = 1;
        sub.pColorAttachments = &colorRef;

        VkRenderPassCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        ci.attachmentCount = 1;
        ci.pAttachments = &color;
        ci.subpassCount = 1;
        ci.pSubpasses = &sub;
        vkCreateRenderPass(device, &ci, nullptr, &renderPass);
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
            VkFramebufferCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ci.renderPass = renderPass;
            ci.attachmentCount = 1;
            ci.pAttachments = &swapChainImageViews[i];
            ci.width = swapChainExtent.width;
            ci.height = swapChainExtent.height;
            ci.layers = 1;
            vkCreateFramebuffer(device, &ci, nullptr, &swapChainFramebuffers[i]);
        }
    }

    void createCommandPool() {
        auto q = findQueues(physicalDevice);
        VkCommandPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        ci.queueFamilyIndex = q.graphics;
        vkCreateCommandPool(device, &ci, nullptr, &commandPool);
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 } };
        VkDescriptorPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        ci.maxSets = 1000;
        ci.poolSizeCount = 1;
        ci.pPoolSizes = pool_sizes;
        vkCreateDescriptorPool(device, &ci, nullptr, &descriptorPool);
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = commandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
        vkAllocateCommandBuffers(device, &ai, commandBuffers.data());
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(swapChainImages.size());

        VkSemaphoreCreateInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fi{};
        fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkCreateSemaphore(device, &si, nullptr, &imageAvailableSemaphores[i]);
            vkCreateFence(device, &fi, nullptr, &inFlightFences[i]);
        }
        for (size_t i = 0; i < swapChainImages.size(); ++i) {
            vkCreateSemaphore(device, &si, nullptr, &renderFinishedSemaphores[i]);
        }
    }

    VkShaderModule loadShader(const std::string& path) {
        std::ifstream f(path, std::ios::ate | std::ios::binary);
        if (!f.is_open()) throw std::runtime_error("Failed to open shader: " + path);
        size_t sz = (size_t)f.tellg();
        std::vector<char> buf(sz);
        f.seekg(0);
        f.read(buf.data(), sz);
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = buf.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(buf.data());
        VkShaderModule m;
        vkCreateShaderModule(device, &ci, nullptr, &m);
        return m;
    }

    void createOverlayPipeline() {
        VkSamplerCreateInfo sci{};
        sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sci.magFilter = VK_FILTER_LINEAR;
        sci.minFilter = VK_FILTER_LINEAR;
        sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(device, &sci, nullptr, &overlaySampler);

        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo dslci{};
        dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dslci.bindingCount = 1;
        dslci.pBindings = &binding;
        vkCreateDescriptorSetLayout(device, &dslci, nullptr, &overlayDescriptorSetLayout);

        std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), overlayDescriptorSetLayout);
        VkDescriptorSetAllocateInfo dsai{};
        dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsai.descriptorPool = descriptorPool;
        dsai.descriptorSetCount = (uint32_t)swapChainImages.size();
        dsai.pSetLayouts = layouts.data();
        overlayDescriptorSets.resize(swapChainImages.size());
        vkAllocateDescriptorSets(device, &dsai, overlayDescriptorSets.data());

        for (size_t i = 0; i < swapChainImages.size(); ++i) {
            VkDescriptorImageInfo ii{ overlaySampler, overlayImageViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = overlayDescriptorSets[i];
            write.dstBinding = 0;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &ii;
            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }

        VkPipelineLayoutCreateInfo plci{};
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.setLayoutCount = 1;
        plci.pSetLayouts = &overlayDescriptorSetLayout;
        vkCreatePipelineLayout(device, &plci, nullptr, &overlayPipelineLayout);

        VkShaderModule vert = loadShader("Shaders/overlay.vert.spv");
        VkShaderModule frag = loadShader("Shaders/overlay.frag.spv");

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vert;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = frag;
        stages[1].pName = "main";

        VkPipelineVertexInputStateCreateInfo vin{};
        vin.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo ia{};
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo vs{};
        vs.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vs.viewportCount = 1;
        vs.pViewports = &viewport;
        vs.scissorCount = 1;
        vs.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rs{};
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.lineWidth = 1.0f;
        rs.cullMode = VK_CULL_MODE_NONE;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState cba{};
        cba.colorWriteMask = 0xF;
        cba.blendEnable = VK_TRUE;
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1;
        cb.pAttachments = &cba;

        VkGraphicsPipelineCreateInfo gp{};
        gp.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gp.stageCount = 2;
        gp.pStages = stages;
        gp.pVertexInputState = &vin;
        gp.pInputAssemblyState = &ia;
        gp.pViewportState = &vs;
        gp.pRasterizationState = &rs;
        gp.pMultisampleState = &ms;
        gp.pColorBlendState = &cb;
        gp.layout = overlayPipelineLayout;
        gp.renderPass = renderPass;
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp, nullptr, &overlayGraphicsPipeline);

        vkDestroyShaderModule(device, frag, nullptr);
        vkDestroyShaderModule(device, vert, nullptr);
    }

    bool init(GLFWwindow* win, uint32_t w, uint32_t h) {
        window = win;
        try {
            createInstance();
            createSurface();
            pickDevice();
            createDevice();
            createSwapchain();
            createImageViews();
            createOverlayImages();
            createRenderPass();
            createFramebuffers();
            createCommandPool();
            createDescriptorPool();
            createOverlayPipeline();
            createCommandBuffers();
            createSyncObjects();

            auto q = findQueues(physicalDevice);
            if (q.graphics == UINT32_MAX) throw std::runtime_error("Invalid queue");

            ctx = nk_glfw3_init(window, device, physicalDevice,
                                q.graphics,
                                overlayImageViews.data(), (uint32_t)overlayImageViews.size(),
                                swapChainImageFormat,
                                NK_GLFW3_INSTALL_CALLBACKS,
                                512 * 1024, 128 * 1024);

            if (!ctx) throw std::runtime_error("Nuklear init failed");

            struct nk_font_atlas* atlas;
            nk_glfw3_font_stash_begin(&atlas);
            struct nk_font* font = nk_font_atlas_add_default(atlas, 16, NULL);
            nk_glfw3_font_stash_end(graphicsQueue);
            nk_style_set_font(ctx, &font->handle);

            startTime = std::chrono::high_resolution_clock::now();
            return true;
        } catch (const std::exception& e) {
            std::cerr << "VkRenderer::init failed: " << e.what() << std::endl;
            return false;
        }
    }

    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult r = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                           imageAvailableSemaphores[currentFrame],
                                           VK_NULL_HANDLE, &imageIndex);
        if (r == VK_ERROR_OUT_OF_DATE_KHR) return;

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        VkSemaphore uiSemaphore = nk_glfw3_render(graphicsQueue, imageIndex,
                                                  imageAvailableSemaphores[currentFrame],
                                                  NK_ANTI_ALIASING_ON);

        VkCommandBuffer cb = commandBuffers[currentFrame];
        vkResetCommandBuffer(cb, 0);

        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cb, &bi);

        // NOTE: No layout barrier needed here. nk_glfw3_render()'s render pass
        // already leaves overlayImages[imageIndex] in SHADER_READ_ONLY_OPTIMAL
        // (finalLayout = SHADER_READ_ONLY_OPTIMAL in nk_glfw3_create_render_pass),
        // which is exactly the layout needed for sampling in the overlay pipeline.

        VkClearValue clear{};
        clear.color = {{0.05f, 0.05f, 0.08f, 1.0f}};

        VkRenderPassBeginInfo rp{};
        rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp.renderPass = renderPass;
        rp.framebuffer = swapChainFramebuffers[imageIndex];
        rp.renderArea = {{0, 0}, swapChainExtent};
        rp.clearValueCount = 1;
        rp.pClearValues = &clear;

        vkCmdBeginRenderPass(cb, &rp, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, overlayGraphicsPipeline);
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, overlayPipelineLayout, 0, 1,
                                &overlayDescriptorSets[imageIndex], 0, nullptr);
        vkCmdDraw(cb, 3, 1, 0, 0);

        vkCmdEndRenderPass(cb);
        vkEndCommandBuffer(cb);

        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        VkSemaphore wait[] = { uiSemaphore }; 
        VkPipelineStageFlags stages[] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };
        
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = wait;
        si.pWaitDstStageMask = stages;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cb;
        
        VkSemaphore signal[] = { renderFinishedSemaphores[imageIndex] };
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = signal;

        vkQueueSubmit(graphicsQueue, 1, &si, inFlightFences[currentFrame]);

        VkPresentInfoKHR pi{};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = signal;
        pi.swapchainCount = 1;
        pi.pSwapchains = &swapChain;
        pi.pImageIndices = &imageIndex;
        vkQueuePresentKHR(presentQueue, &pi);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void drawGameFrame() { drawFrame(); }
    bool shouldClose() const { return glfwWindowShouldClose(window); }
    void pollEvents() { glfwPollEvents(); }
    void waitIdle() { vkDeviceWaitIdle(device); }
    double timeSeconds() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - startTime).count();
    }

    ~VkRendererImpl() {
        vkDeviceWaitIdle(device);
        if (ctx) nk_glfw3_shutdown();

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
        for (auto s : renderFinishedSemaphores) vkDestroySemaphore(device, s, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyPipeline(device, overlayGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, overlayPipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, overlayDescriptorSetLayout, nullptr);
        vkDestroySampler(device, overlaySampler, nullptr);
        
        vkDestroyRenderPass(device, renderPass, nullptr);
        for (auto fb : swapChainFramebuffers) vkDestroyFramebuffer(device, fb, nullptr);
        for (auto iv : overlayImageViews) vkDestroyImageView(device, iv, nullptr);
        for (auto im : overlayImageMemories) vkFreeMemory(device, im, nullptr);
        for (auto img : overlayImages) vkDestroyImage(device, img, nullptr);
        for (auto iv : swapChainImageViews) vkDestroyImageView(device, iv, nullptr);
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
};

VkRenderer::VkRenderer() : impl(new VkRendererImpl()) {}
VkRenderer::~VkRenderer() { delete impl; }
bool VkRenderer::init(GLFWwindow* window, uint32_t w, uint32_t h) { return impl->init(window, w, h); }
void VkRenderer::drawFrame() { impl->drawFrame(); }
void VkRenderer::drawGameFrame() { impl->drawGameFrame(); }
bool VkRenderer::shouldClose() const { return impl->shouldClose(); }
void VkRenderer::pollEvents() { impl->pollEvents(); }
void VkRenderer::waitIdle() { impl->waitIdle(); }
double VkRenderer::timeSeconds() const { return impl->timeSeconds(); }
struct nk_context* VkRenderer::ctx() const { return impl->ctx; }
uint32_t VkRenderer::createMesh(const std::vector<VkVertex>& v, const std::vector<uint32_t>& i) { return 0; }
uint32_t VkRenderer::createTexture(const unsigned char* p, int w, int h) { return 0; }
void VkRenderer::setDrawItem(uint32_t mesh, uint32_t tex) {}
void VkRenderer::clearDrawItems() {}