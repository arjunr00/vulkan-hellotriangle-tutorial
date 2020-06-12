#ifndef HELLO_TRIANGLE_H
#define HELLO_TRIANGLE_H

#include "vulkan/vulkan_core.h"
#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <optional>

class HelloTriangleApplication {
    public:
        /* Run the Vulkan application */
        void run();

    private:
        /* Struct to hold queue family indices and other information */
        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentationFamily;

            bool isComplete() {
                return graphicsFamily.has_value() && presentationFamily.has_value();
            }
        };

        const uint32_t WIDTH = 800;
        const uint32_t HEIGHT = 600;

        /* Enable validation layers only if in debug mode */
        #ifdef NDEBUG
        const bool enableValidationLayers = false;
        #else
        const bool enableValidationLayers = true;
        #endif

        /* Requested validation layers */
        const std::vector<const char *> requestedLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        /* Requested extensions */
        const std::vector<const char *> requestedExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        GLFWwindow *window;  /* The GLFW window */
        VkInstance instance; /* The Vulkan instance */

        VkPhysicalDevice physicalDevice /* The physical device */ = VK_NULL_HANDLE;
        VkDevice         logicalDevice; /* The logical device */

        VkQueue graphicsQueue;     /* A queue to draw graphics */
        VkQueue presentationQueue; /* A queue to present images to the window */

        VkSurfaceKHR surface; /* Window surface for drawing */

        VkDebugUtilsMessengerEXT debugMessenger; /* A debug messenger */

        /* Initialize a GLFW window */
        void initWindow();
        /* Initialize Vulkan instance */
        void initVulkan();
        /* Draw to window */
        void mainLoop();
        /* Terminate Vulkan and GLFW */
        void cleanup();

        /* Create a new Vulkan instance */
        void createInstance();

        /* Check if requested validation layers are available */
        bool checkValidationLayerSupport();
        /* Get list of required extensions */
        std::vector<const char *> getRequiredExtensions();

        /* Create a new window surface */
        void createSurface();

        /* Pick a physical device that supports Vulkan, if it exists */
        void pickPhysicalDevice();
        /* Check whether a given device supports this application */
        bool isDeviceSuitable(VkPhysicalDevice device);

        /* Assign indices to available queue families */
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        /* Create a new logical device to interface with the physical one */
        void createLogicalDevice();

        /* Check whether the physical device supports the requested extensions */
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        /* Populate debug messenger */
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        /* Set up the debug messenger */
        void setupDebugMessenger();

        /* Debug callback function */
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);
        static VkResult CreateDebugUtilsMessengerEXT(
                VkInstance instance,
                const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                const VkAllocationCallbacks *pAllocator,
                VkDebugUtilsMessengerEXT *pDebugMessenger);
        static void DestroyDebugUtilsMessengerEXT(
                VkInstance instance,
                VkDebugUtilsMessengerEXT debugMessenger,
                const VkAllocationCallbacks *pAllocator);
};

#endif
