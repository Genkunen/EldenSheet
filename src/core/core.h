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
    const char** exts;
    uint32_t extsCount;
    uint32_t graphicsQueueFamilyIndex;
};

struct EFrame {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFence fence;
    VkFramebuffer frameBuffer;
    VkImage image;
    VkImageView imageView;
};

struct EFrameSemaphores {
    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
};

struct EDisplay_t {
    EResult result;
    struct EFrame* frames;
    struct EFrameSemaphores* semaphores;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkRenderPass renderPass;
    uint32_t frameCount;
    uint32_t semaphoreCount;
    int width;  // glfw forces int
    int height;
};
