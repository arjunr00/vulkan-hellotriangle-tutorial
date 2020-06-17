#include "HelloTriangle.hpp"
#include "vulkan/vulkan_core.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h>

HelloTriangleApplication::HelloTriangleApplication() {
    /* Initialize window */
    glfwInit();
    
    /* Disable OpenGL, we aren't using it */
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    /* Initialize Vulkan stuff */
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    setupDebugMessenger();
}

HelloTriangleApplication::~HelloTriangleApplication() {
    for (auto framebuffer : swapChainFramebuffers)
        vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
    vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
    for (auto& imageView : swapChainImageViews)
        vkDestroyImageView(logicalDevice, imageView, nullptr);
    vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
    vkDestroyDevice(logicalDevice, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    if (enableValidationLayers) DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void
HelloTriangleApplication::run() {
    /* Keep the window updated */
    while (!glfwWindowShouldClose(window))
        glfwPollEvents();
}

bool
HelloTriangleApplication::checkValidationLayerSupport() {
    uint32_t layerCount;
    /* Find out how many validation layers are available to Vulkan */
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);

    /* Populate vector with available validation layers */
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    /* Check if requested validation layers are present in vector */
    for (const auto& requestedName : requestedLayers) {
        bool layerExists = false;
        
        for (const auto& available : availableLayers)
            if (strcmp(requestedName, available.layerName) == 0) {
                layerExists = true;
                break;
            }

        if (!layerExists) return false;
    }

    return true;
}

std::vector<const char *>
HelloTriangleApplication::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    /* Add debug utils extension if requested */
    if (enableValidationLayers) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

void
HelloTriangleApplication::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("One or more requested validation layers are not available.");

    /* Set up application info (useful for driver) */
    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    /* Tells Vulkan driver what global extensions and validation layers we want */
    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto requiredExtensions = getRequiredExtensions();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    /* If requested, include validation layers */
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(requestedLayers.size());
        createInfo.ppEnabledLayerNames = requestedLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    /* Create the instance */
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance.");
}

void
HelloTriangleApplication::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface.");
}

void
HelloTriangleApplication::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    /* Get number of physical devices */
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("Failed to find any GPUs with Vulkan support.");

    std::vector<VkPhysicalDevice> devices(deviceCount);

    /* Populate vector with available physical devices */
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices)
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }

    if (physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("Failed to find a suitable GPU.");
}

bool
HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device) {
    return (
        findQueueFamilies(device).isComplete() &&
        checkDeviceExtensionSupport(device) &&
        querySwapChainSupport(device).isComplete()
    );
}

HelloTriangleApplication::QueueFamilyIndices
HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    /* Get number of available queue families */
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

    /* Populate vector with available queue families */
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    /* Find a queue family with graphics capabilities and index it */
    for (const auto& family : queueFamilies) {
        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport == VK_TRUE)
            indices.presentationFamily = i;

        if (indices.isComplete()) break;

        ++i;
    }

    return indices;
}

void
HelloTriangleApplication::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    /* Set up required queues */
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(), indices.presentationFamily.value()
    };
    /* Assign the highest priority to the queues */
    float queuePriority = 1.f;
    for (uint32_t family : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = family;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    /* Specify device features */
    VkPhysicalDeviceFeatures deviceFeatures {};
    /* Don't need anything special right now, can leave everything as is */
    (void) deviceFeatures;

    /* Set up the logical device */
    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    /* Add in requested extensions */
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requestedExtensions.size());
    createInfo.ppEnabledExtensionNames = requestedExtensions.data();

    /* Instance and device now have same validation layers; this is for backwards compatibility */
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(requestedLayers.size());
        createInfo.ppEnabledLayerNames = requestedLayers.data();
    } else
        createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device.");

    vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, indices.presentationFamily.value(), 0, &presentationQueue);
}

bool
HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount = 0;
    /* Count the number of available extensions */
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);

    /* Populate the vector with the available extensions */
    vkEnumerateDeviceExtensionProperties
        (device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requestedExtensionSet
                            (requestedExtensions.begin(), requestedExtensions.end());

    for (const auto& extension : availableExtensions)
        requestedExtensionSet.erase(extension.extensionName);

    return requestedExtensionSet.empty();
}

void
HelloTriangleApplication::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.surfaceFormats);
    VkPresentModeKHR presentationMode = chooseSwapPresentationMode(swapChainSupport.presentationModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.surfaceCapabilities);

    /* Add 1 to minimum number of images in chain to give driver time to perform operations */
    uint32_t imageCount = swapChainSupport.surfaceCapabilities.minImageCount + 1;
    if (swapChainSupport.surfaceCapabilities.maxImageCount > 0 &&
            imageCount > swapChainSupport.surfaceCapabilities.maxImageCount)
        imageCount = swapChainSupport.surfaceCapabilities.maxImageCount;

    /* Set up swap chain */
    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format; /* Could be directed to a separate image first */
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; /* Always one unless we're doing stereoscopic 3D */
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(),
                                      indices.presentationFamily.value() };

    if (indices.graphicsFamily != indices.presentationFamily) {
        /* If two different queue families are used, handle them concurrently */
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        /* Otherwise, use them exclusively (faster) */
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    /* Could transform the image (e.g. rotation, flip) */
    createInfo.preTransform = swapChainSupport.surfaceCapabilities.currentTransform;
    /* Can opt to use alpha channel to blend with other windows */
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentationMode;
    createInfo.clipped = VK_TRUE; /* Ignore pixel colors behind window */
    createInfo.oldSwapchain = VK_NULL_HANDLE; /* Swap chain could become invalid or unoptimized */

    if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain.");

    /* Store images and information as member variables */
    vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

HelloTriangleApplication::SwapChainSupportDetails
HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    /* Determine supported capabilities */
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.surfaceCapabilities);

    uint32_t formatCount;
    uint32_t presentationModeCount;
    /* Count the number of available surface formats */
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    /* Count the number of available presentation modes */
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &presentationModeCount, nullptr);

    if (formatCount != 0) {
        details.surfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
                device, surface, &formatCount, details.surfaceFormats.data());
    }

    if (presentationModeCount != 0) {
        details.presentationModes.resize(presentationModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, surface, &presentationModeCount, details.presentationModes.data());
    }

    return details;
}

VkSurfaceFormatKHR
HelloTriangleApplication::chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& format : availableFormats) {
        /* Look for sRGB support */
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    /* Default to first format if "best" couldn't be found */
    return availableFormats[0];
}

VkPresentModeKHR
HelloTriangleApplication::chooseSwapPresentationMode(
        const std::vector<VkPresentModeKHR>& availablePresentationModes) {
    for (const auto& mode : availablePresentationModes) {
        /* Look for mailbox presentation */
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }

    /* Default to regular FIFO presentation, which is guaranteed to be available */
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities) {
    /* Default to resolution automatically set by Vulkan */
    if (capabilities.currentExtent.width != UINT32_MAX)
        return capabilities.currentExtent;

    /* Otherwise, determine the resolution manually */
    VkExtent2D actualExtent = { WIDTH, HEIGHT };

    actualExtent.width =
        std::max(capabilities.minImageExtent.width,
                 std::min(capabilities.maxImageExtent.width,
                          actualExtent.width));
    actualExtent.height =
        std::max(capabilities.minImageExtent.height,
                 std::min(capabilities.maxImageExtent.height,
                          actualExtent.height));

    return actualExtent;
}

void
HelloTriangleApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    int i = 0;
    for (const auto& image : swapChainImages) {
        /* Set up each image view */
        VkImageViewCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; /* How to treat image data (need a 2D image) */
        createInfo.format = swapChainImageFormat;

        /* Can manually control RGBA components */
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        /* Define image's purpose and accessed areas */
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1; /* Multiple layers is for stereographic 3D */

        if (vkCreateImageView(
                    logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create image views.");
        ++i;
    }
}

void
HelloTriangleApplication::createRenderPass() {
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; /* Clear attachment before rendering */
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; /* Retain rendered contents */
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; /* Present image in swap chain */

    /* Specify subpass references */
    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    /* Set up subpasses */
    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass.");
}

void
HelloTriangleApplication::createGraphicsPipeline() {
    std::vector<char> vertShaderBuf = readFile("build/shader.vert.spv");
    std::vector<char> fragShaderBuf = readFile("build/shader.frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderBuf);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderBuf);

    /* Set up vertex shader stage */
    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; /* Specify pipeline stage */
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main"; /* Entrypoint function name */
    vertShaderStageInfo.pSpecializationInfo = nullptr; /* Can be used to specify constant values */

    /* Set up fragment shader stage */
    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; /* Specify pipeline stage */
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main"; /* Entrypoint function name */
    fragShaderStageInfo.pSpecializationInfo = nullptr; /* Can be used to specify constant values */

    VkPipelineShaderStageCreateInfo shaderStages[] = 
        { vertShaderStageInfo, fragShaderStageInfo };

    /* Set up vertex data (i.e. bindings, attributes) */
    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    /* Set up geometry topology information */
    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    /* Draw triangle from every 3 vertices without reuse */
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    /* Can use primitive restart to manually specify indices to be loaded */
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    /* Set up viewport (i.e. tranformation from image to framebuffer) */
    VkViewport viewport {};
    viewport.x = 0.f;
    viewport.y = 0.f;
    /* Swap chain image dimensions may differ from window, but will be used as framebuffers */
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    /* Set up scissors (i.e. region inside which to store pixels) */
    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    /* Set up rasterizer to convert vertex geometry into fragments */
    VkPipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    /* If VK_TRUE, will clamp out-of-bounds fragments to near/far plane instead of discarding */
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE; /* If VK_TRUE, disables rasterization stage */
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; /* Determines how to generate fragments */
    rasterizer.lineWidth = 1.f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; /* Type of face culling */
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; /* Vertex order to determine orientation */
    /* Can manually configure depth values; useful for shadow mapping */
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.f;
    rasterizer.depthBiasClamp = 0.f;
    rasterizer.depthBiasSlopeFactor = 0.f;

    /* Set up multisampling; useful for anti-aliasing */
    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    /* Set up color blending */
    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE; /* If true, can specify bitwise blending operation */
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.f;
    colorBlending.blendConstants[1] = 0.f;
    colorBlending.blendConstants[2] = 0.f;
    colorBlending.blendConstants[3] = 0.f;

    /* Set up pipeline layout for specifying uniform values for shaders at draw time */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout)
            != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout.");

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2; /* One for vertex and fragment shader */
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; /* Can be used to change some state at draw time */
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; /* Can create pipeline from existing one */
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(
                logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline)
            != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline.");

    /* Can destroy these once the graphics pipeline is built */
    vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

VkShaderModule
HelloTriangleApplication::createShaderModule(const std::vector<char>& shaderBuf) {
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderBuf.size();
    /* Reinterpret bytes in buffer as uint32; buffer must satisfy alignment requirements
     * but std::vector ensures this by default */
    createInfo.pCode = reinterpret_cast<const uint32_t *>(shaderBuf.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module.");

    return shaderModule;
}

void
HelloTriangleApplication::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
        VkImageView attachments[] = { swapChainImageViews[i] };

        /* Set up framebuffer */
        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i])
                != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer.");
    }
}

void
HelloTriangleApplication::populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    /* Allow callback to be called for all messages except general debug info */
    createInfo.messageSeverity = // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    /* Enable callback for all message types */
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    /* Point to our callback function */
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}

void
HelloTriangleApplication::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger)
            != VK_SUCCESS)
        throw std::runtime_error("Failed to set up debug messenger.");
}

std::vector<char>
HelloTriangleApplication::readFile(const std::string& filename) {
    /* ate = AT End of file, binary = read in raw bytes */
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("Failed to open file.");

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
HelloTriangleApplication::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {
    (void) pUserData; /* Unused */

    std::string severity;
    std::string type;

    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            severity = "verbose";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            severity = "info";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            severity = "warning";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            severity = "error";
            break;
        default:
            severity = "-";
    }

    switch (messageType) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            type = "general";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            type = "validation";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            type = "performance";
            break;
        default:
            type = "-";
    }

    std::cerr
        << "[" << severity << "] "
        << "[" << type << "] "
        << "Validation layer: "
        << pCallbackData->pMessage
        << std::endl;

    return VK_FALSE;
}

VkResult
HelloTriangleApplication::CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
                    vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    return func != nullptr
            ? func(instance, pCreateInfo, pAllocator, pDebugMessenger)
            : VK_ERROR_EXTENSION_NOT_PRESENT;
}

void
HelloTriangleApplication::DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
                    vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) func(instance, debugMessenger, pAllocator);
}
