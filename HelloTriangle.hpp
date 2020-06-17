#ifndef HELLO_TRIANGLE_H
#define HELLO_TRIANGLE_H

#include "vulkan/vulkan_core.h"
#include <string>
#include <vector>
#include <optional>

#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h>

class HelloTriangleApplication {
    public:
        HelloTriangleApplication();
        ~HelloTriangleApplication();

        /* Run the Vulkan application */
        void run();

    private:
        /* Struct to hold queue family indices */
        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentationFamily;

            bool isComplete() {
                return graphicsFamily.has_value() && presentationFamily.has_value();
            }
        };
        /* Struct to hold swap chain properties */
        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            std::vector<VkSurfaceFormatKHR> surfaceFormats;
            std::vector<VkPresentModeKHR> presentationModes;

            bool isComplete() {
                return !surfaceFormats.empty() && !presentationModes.empty();
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

        /* Bound the number of frames that can be in-flight at a time */
        const size_t MAX_FRAMES_IN_FLIGHT = 2;

        GLFWwindow *window;  /* The GLFW window */
        VkInstance instance; /* The Vulkan instance */

        VkPhysicalDevice physicalDevice /* The physical device */ = VK_NULL_HANDLE;
        VkDevice         logicalDevice; /* The logical device */

        VkQueue graphicsQueue;     /* A queue to draw graphics */
        VkQueue presentationQueue; /* A queue to present images to the window */

        VkSurfaceKHR surface; /* The window surface for drawing */

        VkSwapchainKHR swapChain;             /* The swap chain to buffer images */
        std::vector<VkImage> swapChainImages; /* The images in the swap chain */
        VkFormat swapChainImageFormat;        /* The format of the images */
        VkExtent2D swapChainExtent;           /* The resolution of the images */

        std::vector<VkImageView> swapChainImageViews; /* A view into images in the swap chain */
        std::vector<VkFramebuffer> swapChainFramebuffers; /* The framebuffers for rendering */

        VkRenderPass renderPass; /* The actual render pass */
        VkPipelineLayout pipelineLayout; /* A pipeline layout for shaders */
        VkPipeline graphicsPipeline; /* The graphics pipeline */

        VkCommandPool commandPool; /* A memory pool to manage memory for command buffers */
        std::vector<VkCommandBuffer> commandBuffers; /* The command buffers */

        /* Semaphores for synchronizing drawing threads */
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        /* Fences for CPU-GPU synchronization */
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;

        size_t currentFrame = 0; /* The index of the currently-drawn frame */

        VkDebugUtilsMessengerEXT debugMessenger; /* A debug messenger */

        /* Initialize a GLFW window */
        void initWindow();
        /* Initialize Vulkan instance */
        void initVulkan();
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

        /* Create a new swap chain */
        void createSwapChain();
        /* Find the swap chain properties supported by the physical device */
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        /* Prefer a specific surface format */
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(
                const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats);
        /* Prefer a specific presentation mode */
        VkPresentModeKHR chooseSwapPresentationMode(
                const std::vector<VkPresentModeKHR>& availablePresentationModes);
        /* Prefer the resolution of the window */
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities);
        /* Create a way to access images in the render pipeline */
        void createImageViews();

        /* Create a way to specify framebuffer attachments */
        void createRenderPass();

        /* Create the graphics pipeline */
        void createGraphicsPipeline();
        /* Create a shader module */
        VkShaderModule createShaderModule(const std::vector<char>& shader);

        /* Create framebuffers from swap chain */
        void createFramebuffers();

        /* Create a new command buffer memory pool */
        void createCommandPool();
        /* Create command buffers */
        void createCommandBuffers();

        /* Draw a frame on screen */
        void drawFrame();
        /* Create semaphores and fences for synchronization */
        void createSynchronizationObjs();

        /* Populate debug messenger */
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        /* Set up the debug messenger */
        void setupDebugMessenger();

        /* Read in files */
        static std::vector<char> readFile(const std::string& filename);
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
