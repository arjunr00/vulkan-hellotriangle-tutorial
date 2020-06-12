#include "HelloTriangle.hpp"
#include "vulkan/vulkan_core.h"

#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h>

#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

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
    setupDebugMessenger();
}

void
HelloTriangleApplication::mainLoop() {
    while (!glfwWindowShouldClose(window))
        glfwPollEvents();
}

void
HelloTriangleApplication::cleanup() {
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
    return findQueueFamilies(device).isComplete() && checkDeviceExtensionSupport(device);
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
