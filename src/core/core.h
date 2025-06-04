#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../graphics.h"

struct EWindow_t {
    EResult result;
    GLFWwindow* window;
    struct {
        int width;
        int height;
    } size;
    const char* title;
};

struct EContext_t {
    EResult result;
    VkInstance instance;
#if E_ENABLE_ERROR_CALLBACK
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    VkDescriptorPool descriptorPool;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    const char** exts;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    uint32_t extsCount;
    uint32_t graphicsQueueFamilyIndex;
};
