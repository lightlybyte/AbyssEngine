#include <vkrender.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <iostream>
#include <stdexcept>
#include <array>
#include <vector>
#include <chrono>
#include <algorithm>

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

    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    std::chrono::high_resolution_clock::time_point startTime;

    void createInstance() {
        VkApplicationInfo app{};
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pApplicationName = "Abyss Engine Hub";
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

    bool init(GLFWwindow* win, uint32_t w, uint32_t h) {
        window = win;
        try {
            createInstance();
            createSurface();
            pickDevice();
            createDevice();
            createSwapchain();
            createImageViews();
            createRenderPass();
            createFramebuffers();
            createCommandPool();
            createDescriptorPool();
            createCommandBuffers();
            createSyncObjects();
            
            // Init ImGui Vulkan Backend
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = instance;
            init_info.PhysicalDevice = physicalDevice;
            init_info.Device = device;
            init_info.QueueFamily = findQueues(physicalDevice).graphics;
            init_info.Queue = graphicsQueue;
            init_info.DescriptorPool = descriptorPool;
            init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
            init_info.ImageCount = (uint32_t)swapChainImages.size();
            init_info.RenderPass = renderPass; // Some older ImGui versions need this
            ImGui_ImplVulkan_Init(&init_info, renderPass);

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

        VkCommandBuffer cb = commandBuffers[currentFrame];
        vkResetCommandBuffer(cb, 0);

        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cb, &bi);

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

        // Draw ImGui
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);

        vkCmdEndRenderPass(cb);
        vkEndCommandBuffer(cb);

        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore wait[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
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

    bool shouldClose() const { return glfwWindowShouldClose(window); }
    void pollEvents() { glfwPollEvents(); }
    void waitIdle() { vkDeviceWaitIdle(device); }
    double timeSeconds() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - startTime).count();
    }

    ~VkRendererImpl() {
        vkDeviceWaitIdle(device);
        ImGui_ImplVulkan_Shutdown();
        
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
        for (auto s : renderFinishedSemaphores) vkDestroySemaphore(device, s, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        for (auto fb : swapChainFramebuffers) vkDestroyFramebuffer(device, fb, nullptr);
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
bool VkRenderer::shouldClose() const { return impl->shouldClose(); }
void VkRenderer::pollEvents() { impl->pollEvents(); }
void VkRenderer::waitIdle() { impl->waitIdle(); }
double VkRenderer::timeSeconds() const { return impl->timeSeconds(); }

// Stubs for the 3D engine (to be re-integrated later)
uint32_t VkRenderer::createMesh(const std::vector<VkVertex>& v, const std::vector<uint32_t>& i) { return 0; }
uint32_t VkRenderer::createTexture(const unsigned char* p, int w, int h) { return 0; }
void VkRenderer::setDrawItem(uint32_t mesh, uint32_t tex) {}
void VkRenderer::clearDrawItems() {}