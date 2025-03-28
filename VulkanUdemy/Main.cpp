#define NOMINMAX

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffer();
        createSyncObjects();

    }


    void createSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }

    }
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) 
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue clearColor = { {{0.07f, 0.47f, 0.95f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

    }

    void createCommandBuffer()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void createCommandPool() 
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }

    void cleanup() {

        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroyFence(device, inFlightFence, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void drawFrame() 
    {
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

        vkResetFences(device, 1, &inFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(commandBuffer, 0);

        recordCommandBuffer(commandBuffer, imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // next two are parallel indices
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
    
        presentInfo.pResults = nullptr; // Optional
        vkQueuePresentKHR(presentQueue, &presentInfo);
    }


    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("assets/shaders/bytecodes/vert_bare.spv");
        auto fragShaderCode = readFile("assets/shaders/bytecodes/frag_bare.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//#define NOMINMAX
//
//#define VK_USE_PLATFORM_WIN32_KHR
//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <GLFW/glfw3native.h>
//#include <iostream>
//#include <stdexcept>
//#include <vector>
//#include "src/core/VRenderer.h"
//#include <cstdlib>
//#include <cstdint> // Necessary for uint32_t
//#include <limits> // Necessary for std::numeric_limits
//#include <algorithm> // Necessary for std::clamp
//
//#include <string>
//#include <optional>
//
//#include <set>
//#include <fstream>
//
//const std::vector<const char*> validationLayersMain = {
//	"VK_LAYER_KHRONOS_validation"
//};
//
//#ifdef NDEBUG
//const bool enableValidationLayersMain = false;
//#else
//const bool enableValidationLayersMain = true;
//#endif
//
//const std::vector<const char*> deviceExtensionsMain = {
//	VK_KHR_SWAPCHAIN_EXTENSION_NAME
//};
//
//
//
//static std::vector<char> readFile(const std::string& filename) {
//	std::ifstream file(filename, std::ios::ate | std::ios::binary);
//
//	if (!file.is_open()) {
//		throw std::runtime_error("failed to open file!");
//	}
//	size_t fileSize = (size_t)file.tellg();
//	std::vector<char> buffer(fileSize);
//	file.seekg(0);
//	file.read(buffer.data(), fileSize);
//
//	file.close();
//
//	return buffer;
//}
//
//GLFWwindow* window;
//VRenderer renderer;
//
//
//VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
//	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
//	if (func != nullptr) {
//		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
//	}
//	else {
//		return VK_ERROR_EXTENSION_NOT_PRESENT;
//	}
//}
//
//void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
//	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
//	if (func != nullptr) {
//		func(instance, debugMessenger, pAllocator);
//	}
//}
//
//class HelloTriangleApp
//{
//	const uint32_t _width{ 800 }, _height{ 600 };
//	const std::string _title{ "My Vulkan App"};
//
//	VkInstance _instance;
//	VkDebugUtilsMessengerEXT _debugMessenger;
//	VkSurfaceKHR _surface;
//	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
//	VkDevice _logDevice = VK_NULL_HANDLE;
//	VkQueue _graphicsQueue = VK_NULL_HANDLE;
//	VkQueue _presentQueue = VK_NULL_HANDLE;
//	VkSwapchainKHR _swapChain;
//	std::vector<VkImage> _swapChainImages;
//	VkFormat _swapChainImageFormat;
//	VkExtent2D _swapChainExtent;
//	std::vector<VkImageView> _swapChainImageViews;
//	VkRenderPass _renderPass;
//	VkPipelineLayout _pipelineLayout;
//
//	VkPipeline _graphicsPipeline;
//
//	//std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // false
//
//	//graphicsFamily = 0;
//
//	//std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // true
//
//	VkResult initializeVulkan() {
//
//
//		createInstance();
//		setupDebugMessenger();
//		createSurface();
//
//		pickPhysicalDevice();
//		createLogicalDevice();
//		createSwapChain();
//		createImageViews();
//		createRenderPass();
//		createGraphicsPipeline();
//		
//		return VK_SUCCESS;
//	}
//
//	VkResult cleanup() {
//		vkDestroyPipeline(_logDevice, _graphicsPipeline, nullptr);
//		vkDestroyPipelineLayout(_logDevice, _pipelineLayout, nullptr);
//		vkDestroyRenderPass(_logDevice, _renderPass, nullptr);
//
//		for (auto imageView : _swapChainImageViews) {
//			vkDestroyImageView(_logDevice, imageView, nullptr);
//		}
//
//		vkDestroySwapchainKHR(_logDevice, _swapChain, nullptr);
//
//		if (_logDevice != VK_NULL_HANDLE)
//			vkDestroyDevice(_logDevice, nullptr);
//
//		if (enableValidationLayersMain) {
//			DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
//		}
//
//		vkDestroySurfaceKHR(_instance, _surface, nullptr);
//		vkDestroyInstance(_instance, nullptr);
//
//		glfwDestroyWindow(window);
//
//		glfwTerminate();
//		return VK_SUCCESS;
//	}
//
//	void createRenderPass()
//	{
//		VkAttachmentDescription colorAttachment{};
//		colorAttachment.format = _swapChainImageFormat;
//		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//		VkAttachmentReference colorAttachmentRef{};
//		colorAttachmentRef.attachment = 0;
//		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//		VkSubpassDescription subpass{};
//		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//
//		subpass.colorAttachmentCount = 1;
//		subpass.pColorAttachments = &colorAttachmentRef;
//
//		VkRenderPassCreateInfo renderPassInfo{};
//		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//		renderPassInfo.attachmentCount = 1;
//		renderPassInfo.pAttachments = &colorAttachment;
//		renderPassInfo.subpassCount = 1;
//		renderPassInfo.pSubpasses = &subpass;
//
//		if (vkCreateRenderPass(_logDevice, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create render pass!");
//		}
//	}
//
//	VkShaderModule createShaderModule(const std::vector<char>& code) 
//	{
//		VkShaderModuleCreateInfo createInfo{};
//		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//		createInfo.codeSize = code.size();
//		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
//		VkShaderModule shaderModule;
//		if (vkCreateShaderModule(_logDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create shader module!");
//		}
//		return shaderModule;
//
//	}
//
//	void createGraphicsPipeline()
//	{
//		auto vertShaderCode = readFile("assets/shaders/bytecodes/vert_bare.spv");
//		auto fragShaderCode = readFile("assets/shaders/bytecodes/frag_bare.spv");
//
//		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
//		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
//
//		vkDestroyShaderModule(_logDevice, fragShaderModule, nullptr);
//		vkDestroyShaderModule(_logDevice, vertShaderModule, nullptr);
//
//		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
//		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//
//		vertShaderStageInfo.module = vertShaderModule;
//		vertShaderStageInfo.pName = "main";
//
//		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
//		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//		fragShaderStageInfo.module = fragShaderModule;
//		fragShaderStageInfo.pName = "main";
//
//
//		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
//
//		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//		vertexInputInfo.vertexBindingDescriptionCount = 0;
//		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
//		vertexInputInfo.vertexAttributeDescriptionCount = 0;
//		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
//
//		// Standard Triangle topology setup
//		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//		inputAssembly.primitiveRestartEnable = VK_FALSE;
//
//		VkViewport viewport{};
//		viewport.x = 0.0f;
//		viewport.y = 0.0f;
//		viewport.width = (float)_swapChainExtent.width;
//		viewport.height = (float)_swapChainExtent.height;
//		viewport.minDepth = 0.0f;
//		viewport.maxDepth = 1.0f;
//
//		// cover framebuffer fully
//		VkRect2D scissor{};
//		scissor.offset = { 0, 0 };
//		scissor.extent = _swapChainExtent;
//
//
//		std::vector<VkDynamicState> dynamicStates = {
//			VK_DYNAMIC_STATE_VIEWPORT,
//			VK_DYNAMIC_STATE_SCISSOR
//		};
//
//		VkPipelineDynamicStateCreateInfo dynamicState{};
//		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
//		dynamicState.pDynamicStates = dynamicStates.data();
//
//		VkPipelineViewportStateCreateInfo viewportState{};
//		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//		viewportState.viewportCount = 1;
//		viewportState.pViewports = &viewport;
//		viewportState.scissorCount = 1;
//		viewportState.pScissors = &scissor;
//
//		// rasterize the vertices into fragments
//		VkPipelineRasterizationStateCreateInfo rasterizer{};
//		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//		rasterizer.depthClampEnable = VK_FALSE;
//
//		rasterizer.rasterizerDiscardEnable = VK_FALSE;
//		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//		rasterizer.lineWidth = 1.0f;
//		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
//		rasterizer.depthBiasEnable = VK_FALSE;
//		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
//		rasterizer.depthBiasClamp = 0.0f; // Optional
//		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
//
//		// Anti-aliasing
//		VkPipelineMultisampleStateCreateInfo multisampling{};
//		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//		multisampling.sampleShadingEnable = VK_FALSE;
//		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//		multisampling.minSampleShading = 1.0f; // Optional
//		multisampling.pSampleMask = nullptr; // Optional
//		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
//		multisampling.alphaToOneEnable = VK_FALSE; // Optional	}
//
//		// depth/stencil buffer
//		// none
//
//		// blending (transparency)
//		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//		colorBlendAttachment.blendEnable = VK_FALSE;
//		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
//		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
//
//		// Yes Transparency alpha channel
//		//colorBlendAttachment.blendEnable = VK_TRUE;
//		//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//		//colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//		//colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
//		//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
//		//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//		//colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
//
//
//		VkPipelineColorBlendStateCreateInfo colorBlending{};
//		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//		colorBlending.logicOpEnable = VK_FALSE; //set to true when using alpha blending
//		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
//		colorBlending.attachmentCount = 1;
//		colorBlending.pAttachments = &colorBlendAttachment;
//		colorBlending.blendConstants[0] = 0.0f; // Optional
//		colorBlending.blendConstants[1] = 0.0f; // Optional
//		colorBlending.blendConstants[2] = 0.0f; // Optional
//		colorBlending.blendConstants[3] = 0.0f; // Optional
//
//		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//		pipelineLayoutInfo.setLayoutCount = 0; // Optional
//		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
//		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
//		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
//		
//	
//		if (vkCreatePipelineLayout(_logDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create pipeline layout!");
//		}
//
//		VkGraphicsPipelineCreateInfo pipelineInfo{};
//		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//		pipelineInfo.stageCount = 2;
//		pipelineInfo.pStages = shaderStages;
//
//		pipelineInfo.pVertexInputState = &vertexInputInfo;
//		pipelineInfo.pInputAssemblyState = &inputAssembly;
//		pipelineInfo.pViewportState = &viewportState;
//		pipelineInfo.pRasterizationState = &rasterizer;
//		pipelineInfo.pMultisampleState = &multisampling;
//		pipelineInfo.pDepthStencilState = nullptr; // Optional
//		pipelineInfo.pColorBlendState = &colorBlending;
//		pipelineInfo.pDynamicState = &dynamicState;
//
//		pipelineInfo.layout = _pipelineLayout;
//
//		pipelineInfo.renderPass = _renderPass;
//		pipelineInfo.subpass = 0;
//
//		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
//		pipelineInfo.basePipelineIndex = -1; // Optional
//
//		if (vkCreateGraphicsPipelines(_logDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create graphics pipeline!");
//		}
//	}
//
//	struct QueueFamilyIndices {
//		std::optional<uint32_t> graphicsFamily;
//		std::optional<uint32_t> presentFamily;
//
//		bool isComplete() {
//			return graphicsFamily.has_value() && presentFamily.has_value();
//		}
//	};
//	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
//		QueueFamilyIndices indices;
//
//		
//		uint32_t queueFamilyCount = 0;
//		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
//
//		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
//		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
//
//		int i = 0;
//		for (const auto& queueFamily : queueFamilies) {
//			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
//				indices.graphicsFamily = static_cast<uint32_t>(i);
//			}
//			VkBool32 presentSupport = false;
//			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
//
//			if (presentSupport) {
//				indices.presentFamily = i;
//			}
//
//			if (indices.isComplete()) {
//				break;
//			}
//			i++;
//		}
//
//		
//
//			return indices;
//	}
//
//
//	struct SwapChainSupportDetails {
//		VkSurfaceCapabilitiesKHR capabilities;
//		std::vector<VkSurfaceFormatKHR> formats;
//		std::vector<VkPresentModeKHR> presentModes;
//	};
//
//	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
//		SwapChainSupportDetails details;
//
//		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);
//
//		uint32_t formatCount;
//		vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);
//
//		if (formatCount != 0) {
//			details.formats.resize(formatCount);
//			vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
//		}
//		
//		uint32_t presentModeCount;
//		vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);
//
//		if (presentModeCount != 0) {
//			details.presentModes.resize(presentModeCount);
//			vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
//		}
//
//		return details;
//	}
//
//
//	
//
//	void   createImageViews()
//	{
//		_swapChainImageViews.resize(_swapChainImages.size());
//
//		for (size_t i = 0; i < _swapChainImages.size(); i++) {
//
//
//
//			VkImageViewCreateInfo createInfo{};
//			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//			createInfo.image = _swapChainImages[i];
//
//			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//			createInfo.format = _swapChainImageFormat;
//
//			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
//			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
//			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
//			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
//
//			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//			createInfo.subresourceRange.baseMipLevel = 0;
//			createInfo.subresourceRange.levelCount = 1;
//			createInfo.subresourceRange.baseArrayLayer = 0;
//			createInfo.subresourceRange.layerCount = 1;
//
//			if (vkCreateImageView(_logDevice, &createInfo, nullptr, &_swapChainImageViews[i]) != VK_SUCCESS) {
//				throw std::runtime_error("failed to create image views!");
//			}
//
//		}
//	}
//
//	void createSwapChain() {
//		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice);
//
//		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
//		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
//		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
//
//		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
//
//		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
//			imageCount = swapChainSupport.capabilities.maxImageCount;
//		}
//
//		VkSwapchainCreateInfoKHR createInfo{};
//		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
//		createInfo.surface = _surface;
//
//		createInfo.minImageCount = imageCount;
//		createInfo.imageFormat = surfaceFormat.format;
//		createInfo.imageColorSpace = surfaceFormat.colorSpace;
//		createInfo.imageExtent = extent;
//		createInfo.imageArrayLayers = 1;
//		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
//
//		QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
//		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
//
//		if (indices.graphicsFamily != indices.presentFamily) {
//			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
//			createInfo.queueFamilyIndexCount = 2;
//			createInfo.pQueueFamilyIndices = queueFamilyIndices;
//		}
//		else {
//			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
//			createInfo.queueFamilyIndexCount = 0; // Optional
//			createInfo.pQueueFamilyIndices = nullptr; // Optional
//		}
//
//		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
//		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
//		createInfo.presentMode = presentMode;
//		createInfo.clipped = VK_TRUE;
//		createInfo.oldSwapchain = VK_NULL_HANDLE;
//
//		if (vkCreateSwapchainKHR(_logDevice, &createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create swap chain!");
//		}
//
//		vkGetSwapchainImagesKHR(_logDevice, _swapChain, &imageCount, nullptr);
//		_swapChainImages.resize(imageCount);
//		vkGetSwapchainImagesKHR(_logDevice, _swapChain, &imageCount, _swapChainImages.data());
//
//		_swapChainImageFormat = surfaceFormat.format;
//		_swapChainExtent = extent;
//	}
//
//
//	void createSurface()
//	{
//		//VkWin32SurfaceCreateInfoKHR createInfo{};
//		//createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
//		//createInfo.hwnd = glfwGetWin32Window(window);
//		//createInfo.hinstance = GetModuleHandle(nullptr);
//		//
//		//if (vkCreateWin32SurfaceKHR(_instance, &createInfo, nullptr, &_surface) != VK_SUCCESS) {
//		//	throw std::runtime_error("failed to create window surface!");
//		//}
//
//		if (glfwCreateWindowSurface(_instance, window, nullptr, &_surface) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create window surface!");
//		}
//	}
//
//	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
//		uint32_t extensionCount;
//		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
//
//		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
//		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
//
//		std::set<std::string> requiredExtensions(deviceExtensionsMain.begin(), deviceExtensionsMain.end());
//
//		for (const auto& extension : availableExtensions) {
//			requiredExtensions.erase(extension.extensionName);
//		}
//
//		return requiredExtensions.empty();
//	}
//
//	void pickPhysicalDevice()
//	{
//		uint32_t deviceCount = 0;
//		vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
//
//		if (deviceCount == 0) {
//			throw std::runtime_error("failed to find GPUs with Vulkan support!");
//		}
//
//		std::vector<VkPhysicalDevice> devices(deviceCount);
//		vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
//
//		for (const auto& device : devices) {
//			if (isDeviceSuitable(device)) {
//				_physicalDevice = device;
//				break;
//			}
//		}
//
//		if (_physicalDevice == VK_NULL_HANDLE) {
//			throw std::runtime_error("failed to find a suitable GPU!");
//		}
//
//
//	}
//
//	bool isDeviceSuitable(VkPhysicalDevice device) {
//		VkPhysicalDeviceProperties deviceProperties;
//		VkPhysicalDeviceFeatures deviceFeatures;
//		vkGetPhysicalDeviceProperties(device, &deviceProperties);
//		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
//
//		QueueFamilyIndices indices = findQueueFamilies(device);
//
//		bool extensionsSupported = checkDeviceExtensionSupport(device);
//
//		bool swapChainAdequate = false;
//		if (extensionsSupported) {
//			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
//			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
//		}
//
//		return indices.isComplete() && extensionsSupported && swapChainAdequate;
//	}
//
//
//
//	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&availableFormats) {
//		for (const auto& availableFormat : availableFormats) {
//			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
//				return availableFormat;
//			}
//		}
//
//		return availableFormats[0];
//	}
//
//	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
//		for (const auto& availablePresentMode : availablePresentModes) {
//			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
//				return availablePresentMode;
//			}
//		}
//
//		return VK_PRESENT_MODE_FIFO_KHR;
//	}
//
//	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
//		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
//			return capabilities.currentExtent;
//		}
//		else {
//			int width, height;
//			glfwGetFramebufferSize(window, &width, &height);
//
//			VkExtent2D actualExtent = {
//				static_cast<uint32_t>(width),
//				static_cast<uint32_t>(height)
//			};
//
//			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
//			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
//
//			return actualExtent;
//		}
//	}
//
//	void createLogicalDevice()
//	{
//		QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
//		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
//		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
//
//		float queuePriority = 1.0f;
//		for (uint32_t queueFamily : uniqueQueueFamilies) {
//			VkDeviceQueueCreateInfo queueCreateInfo{};
//			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//			queueCreateInfo.queueFamilyIndex = queueFamily;
//			queueCreateInfo.queueCount = 1;
//			queueCreateInfo.pQueuePriorities = &queuePriority;
//			queueCreateInfos.push_back(queueCreateInfo);
//		}
//		
//		/*VkDeviceQueueCreateInfo queueCreateInfo{};
//		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
//		queueCreateInfo.queueCount = 1;
//
//		float queuePriority = 1.0f;
//		queueCreateInfo.pQueuePriorities = &queuePriority;*/
//
//
//		VkDeviceCreateInfo createInfo{};
//		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
//
//		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
//		createInfo.pQueueCreateInfos = queueCreateInfos.data();
//
//		VkPhysicalDeviceFeatures deviceFeatures{};
//		createInfo.pEnabledFeatures = &deviceFeatures;
//
//		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensionsMain.size());
//		createInfo.ppEnabledExtensionNames = deviceExtensionsMain.data();
//
//		if (enableValidationLayersMain) {
//			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersMain.size());
//			createInfo.ppEnabledLayerNames = validationLayersMain.data();
//		}
//		else {
//			createInfo.enabledLayerCount = 0;
//		}
//		
//		if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_logDevice) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create logical device!");
//		}
//
//
//
//		vkGetDeviceQueue(_logDevice, indices.presentFamily.value(), 0, &_presentQueue);
//		vkGetDeviceQueue(_logDevice, indices.graphicsFamily.value(), 0, &_graphicsQueue);
//
//	}
//
//
//	void setupDebugMessenger() {
//		if (!enableValidationLayersMain) return;
//
//		VkDebugUtilsMessengerCreateInfoEXT createInfo;
//		populateDebugMessengerCreateInfo(createInfo);
//
//		if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
//			throw std::runtime_error("failed to set up debug messenger!");
//		}
//	}
//
//	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
//		createInfo = {};
//		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
//		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
//		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
//		createInfo.pfnUserCallback = debugCallback;
//	}
//
//	bool initWindow() { 
//		glfwInit();
//
//		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
//
//		window = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);
//
//		
//		return true; 
//	}
//
//	bool checkValidationLayerSupport() {
//		uint32_t layerCount;
//		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
//
//		std::vector<VkLayerProperties> availableLayers(layerCount);
//		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
//		for (const char* layerName : validationLayersMain) {
//			bool layerFound = false;
//
//			for (const auto& layerProperties : availableLayers) {
//				if (strcmp(layerName, layerProperties.layerName) == 0) {
//					layerFound = true;
//					break;
//				}
//			}
//
//			if (!layerFound) {
//				return false;
//			}
//		}
//
//		return true;
//	}
//
//	VkResult createInstance()
//	{
//		VkApplicationInfo appInfo{};
//		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
//		appInfo.pApplicationName = "Hello Triangle";
//		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
//		appInfo.pEngineName = "No Engine";
//		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
//		appInfo.apiVersion = VK_API_VERSION_1_0;
//
//
//
//		VkInstanceCreateInfo createInfo{};
//		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
//		createInfo.pApplicationInfo = &appInfo;
//
//		
//		auto extensions = getRequiredExtensions();
//		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
//		createInfo.ppEnabledExtensionNames = extensions.data();
//		
//
//
//		if (enableValidationLayersMain && !checkValidationLayerSupport()) {
//			throw std::runtime_error("validation layers requested, but not available!");
//		}
//
//		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
//		if (enableValidationLayersMain) {
//			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersMain.size());
//			createInfo.ppEnabledLayerNames = validationLayersMain.data();
//
//			populateDebugMessengerCreateInfo(debugCreateInfo);
//			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
//		}
//		else {
//			createInfo.enabledLayerCount = 0;
//
//			createInfo.pNext = nullptr;
//		}
//
//		if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create instance!");
//		}
//
//		return VK_SUCCESS;
//	}
//
//	std::vector<const char*> getRequiredExtensions() {
//		uint32_t glfwExtensionCount = 0;
//		const char** glfwExtensions;
//		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//
//		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
//
//		if (enableValidationLayersMain) {
//			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//		}
//
//		return extensions;
//	}
//
//	VkResult mainLoop() {
//		while (!glfwWindowShouldClose(window)) {
//			glfwPollEvents();
//		}
//		return VK_SUCCESS; 
//	}
//
//
//	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
//		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//		VkDebugUtilsMessageTypeFlagsEXT messageType,
//		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
//		void* pUserData) {
//
//		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
//
//		return VK_FALSE;
//	}
//
//public:
//	VkResult run() {
//		if (bool passed = initWindow() == false)
//		{
//			return VK_ERROR_DEVICE_LOST;
//		}
//
//		VkResult result = initializeVulkan();
//		if (result != VK_SUCCESS)
//		{
//			return result;
//		}
//
//		result = mainLoop();
//		if (result != VK_SUCCESS)
//		{
//			return result;
//		}
//
//		result = cleanup();
//		if (result != VK_SUCCESS)
//		{
//			return result;
//		}
//
//		return VK_SUCCESS;
//
//	}
//};
//
//int main()
//{
//	HelloTriangleApp app{};
//	VkResult result = VK_SUCCESS;
//	try {
//		result = app.run();
//	}
//	catch (const std::exception& e)
//	{
//		std::cout << "\n" << e.what() << "\n Line: " << __LINE__ << "\nFile: " << __FILE__ << "\nFunction: " << __FUNCTION__;
//	}
//	if (result != VK_SUCCESS)
//		return EXIT_FAILURE;
//	else
//		return EXIT_SUCCESS;
//}
//
////void InitWindow(std::string name_ = "Test Window", const int width_ = 800, const int height_ = 600);
//
////int main(int argc, char* argv)
////{
////	InitWindow("TestWindow", 800, 600);
////
////	if (renderer.init(window) == EXIT_FAILURE)
////	{	
////		return EXIT_FAILURE;
////	}
////
////	while (!glfwWindowShouldClose(window))
////	{
////		glfwPollEvents();
////	}
////
////	renderer.cleanup();
////
////
////	glfwDestroyWindow(window);
////	glfwTerminate();
////
////	return EXIT_SUCCESS;
////}
////
////void InitWindow(std::string name_, const int width_, const int height_)
////{
////	glfwInit();
////
////	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
////	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
////
////	window = glfwCreateWindow(width_, height_, name_.c_str(), nullptr, nullptr);
////
////
////}