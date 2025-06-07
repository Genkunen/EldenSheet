#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../graphics.h"

struct EWindow_t {
    EResult result;
    GLFWwindow* window;
    const char* title;
    struct {
        int width;
        int height;
    } size;
    int shouldResize;
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
    VkClearValue clearValue;
    uint32_t frameCount;
    uint32_t frameCurrentIndex;
    uint32_t semaphoreCount;
    uint32_t semaphoreCurrentIndex;
    int width;  // glfw forces int
    int height;
};

struct ETexture_t {
    VkDeviceMemory memory;
    VkImage image;
    VkImageView imageView;
    VkDescriptorSet descriptorSet;
};

struct ERenderer_t {
    EResult result;
    VkSampler sampler;
    VkDescriptorSetLayout descSetLayout;
    VkDescriptorPool descPool;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkShaderModule vertShader;
    VkShaderModule fragShader;
    uint32_t descPoolSize;
    ETexture texture;
};
