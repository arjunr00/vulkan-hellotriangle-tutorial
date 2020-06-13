#include "HelloTriangle.hpp"
#include "vulkan/vulkan_core.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h>

void
HelloTriangleApplication::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void
HelloTriangleApplication::initWindow() {
    glfwInit();
    
    /* Disable OpenGL, we aren't using it */
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}


void
HelloTriangleApplication::initVulkan() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    setupDebugMessenger();
}

void
HelloTriangleApplication::mainLoop() {
    while (!glfwWindowShouldClose(window))
        glfwPollEvents();
}

void
HelloTriangleApplication::cleanup() {
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
HelloTriangleApplication::createGraphicsPipeline() {

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
