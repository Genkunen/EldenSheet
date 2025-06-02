#include "instance.h"
#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct EInstance_t {
    EResult result;
    VkInstance instance;
    const char** exts;
#if E_ENABLE_ERROR_CALLBACK
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    VkDescriptorPool descriptorPool;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    uint32_t extsCount;
    uint32_t graphicsQueueFamilyIndex;
};

#if E_ENABLE_ERROR_CALLBACK
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageTypes,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData);

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks* pAllocator,
  VkDebugUtilsMessengerEXT* pDebugMessenger);

static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
  VkDebugUtilsMessengerEXT debugMessenger,
  const VkAllocationCallbacks* pAllocator);
#endif

static void SelectPhysicalDevice(EInstance instance);
static void SelectGraphicsQueueFamilyIndex(EInstance instance);
static void CreateLogicalDevice(EInstance instance);
static void CreateDescriptorPool(EInstance instance);
static void CreateInstance(EInstance instance);
static void CreateWindowSurface(EInstance instance, EWindow window);
static void SelectSurfaceFormat(EInstance instance);

// VkInstance initialization
E_EXTERN void eCreateInstance(EInstance instanceOut[static 1], EWindow window) {
    if (!instanceOut) {
        return;
    }
    EInstance instance = malloc(sizeof(*instance));
    if (!instance) {
        *instanceOut = NULL;
        return;
    }
    *instanceOut = instance;
    *instance = (struct EInstance_t){ 0 };

    CreateInstance(instance);
    SelectPhysicalDevice(instance);
    SelectGraphicsQueueFamilyIndex(instance);
    CreateLogicalDevice(instance);
    CreateDescriptorPool(instance);
    CreateWindowSurface(instance, window);
    SelectSurfaceFormat(instance);
}

// cleanup
E_EXTERN void eDestroyInstance(EInstance instance) {
    free(instance->exts);
#if E_ENABLE_ERROR_CALLBACK
    DestroyDebugUtilsMessengerEXT(
      instance->instance, instance->debugMessenger, NULL);
#endif
    vkDestroySurfaceKHR(instance->instance, instance->surface, NULL);
    vkDestroyDescriptorPool(instance->device, instance->descriptorPool, NULL);
    vkDestroyDevice(instance->device, NULL);
    vkDestroyInstance(instance->instance, NULL);
    free(instance);
}

static void SelectSurfaceFormat(EInstance instance) {
    if (instance->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };
    VkBool32 res = { 0 };
    err = vkGetPhysicalDeviceSurfaceSupportKHR(instance->physicalDevice,
      instance->graphicsQueueFamilyIndex,
      instance->surface,
      &res);
    if (err != VK_SUCCESS || res != VK_TRUE) {
        instance->result = E_NO_AVAILABLE_WSI_SUPPORT;
        return;
    }
    const VkFormat reqFmts[4] = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_R8G8B8_UNORM,
    };
    const VkColorSpaceKHR reqColorSpace = { VK_COLORSPACE_SRGB_NONLINEAR_KHR };

    uint32_t srfFmtCount = { 0 };
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(
      instance->physicalDevice, instance->surface, &srfFmtCount, NULL);
    if (err != VK_SUCCESS) {
        instance->result = E_ENUMERATE_FAILURE;
        return;
    }
    VkSurfaceFormatKHR* srfFmts = malloc(sizeof(*srfFmts) * srfFmtCount);
    if (!srfFmts) {
        instance->result = E_MALLOC_FAILURE;
        return;
    }
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(
      instance->physicalDevice, instance->surface, &srfFmtCount, srfFmts);
    if (err != VK_SUCCESS) {
        instance->result = E_ENUMERATE_FAILURE;
        goto return_early;
    }

    if (srfFmtCount == 1) {
        if (srfFmts[0].format == VK_FORMAT_UNDEFINED) {
            instance->surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
            instance->surfaceFormat.colorSpace =
              VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            goto return_early;
        }
        instance->surfaceFormat = srfFmts[0];
        goto return_early;
    }

    VkSurfaceFormatKHR* fmt = { NULL };
    const VkFormat* reqFmt = { NULL };
    uint32_t reqFmtSize = sizeof(reqFmts) / sizeof(*reqFmts);

    // search if requested format is found
    for (fmt = srfFmts; fmt != srfFmts + srfFmtCount; ++fmt) {
        for (reqFmt = reqFmts; reqFmt != reqFmts + reqFmtSize; reqFmt++) {
            if (fmt->format == *reqFmt && fmt->colorSpace == reqColorSpace) {
                instance->surfaceFormat = *fmt;
                goto return_early;
            }
        }
    }
    // if none found use whatever first available
    instance->surfaceFormat = *srfFmts;
return_early:
    free(srfFmts);
}

static void CreateWindowSurface(EInstance instance, EWindow window) {
    if (instance->result != E_SUCCESS) {
        return;
    }
    VkResult err = glfwCreateWindowSurface(
      instance->instance, eGetGlfwWindow(window), NULL, &instance->surface);
    if (err != VK_SUCCESS) {
        instance->result = E_GLFW_FAILURE;
    }
}

static void SelectPhysicalDevice(EInstance instance) {
    if (instance->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    uint32_t deviceCount = { 0 };
    err = vkEnumeratePhysicalDevices(instance->instance, &deviceCount, NULL);
    if (err != VK_SUCCESS) {
        instance->result = E_ENUMERATE_FAILURE;
        return;
    }
    VkPhysicalDevice* devices = malloc(sizeof(*devices) * deviceCount);
    if (!devices) {
        instance->result = E_MALLOC_FAILURE;
        return;
    }
    err = vkEnumeratePhysicalDevices(instance->instance, &deviceCount, devices);
    if (err != VK_SUCCESS) {
        instance->result = E_ENUMERATE_FAILURE;
        free(devices);
        return;
    }
    VkPhysicalDeviceProperties prop = { 0 };
    for (int i = 0; i < deviceCount; ++i) {
        vkGetPhysicalDeviceProperties(devices[i], &prop);
        if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            instance->physicalDevice = devices[i];
            break;
        }
    }
    if (deviceCount > 0) {
        instance->physicalDevice = devices[0];
    }
    else {
        instance->result = E_NO_AVAILABLE_PHYSICAL_DEVICES;
    }
    free(devices);
}

static void SelectGraphicsQueueFamilyIndex(EInstance instance) {
    if (instance->result != E_SUCCESS) {
        return;
    }
    uint32_t count = { 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(
      instance->physicalDevice, &count, NULL);
    VkQueueFamilyProperties* props = malloc(sizeof(*props) * count);
    if (!props) {
        instance->result = E_MALLOC_FAILURE;
        return;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(
      instance->physicalDevice, &count, props);
    for (int i = 0; i < count; ++i) {
        if (props->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            instance->graphicsQueueFamilyIndex = i;
            return;
        }
    }
    instance->result = E_NO_AVAILABLE_GRAPHICS_QUEUES;
}

static void CreateLogicalDevice(EInstance instance) {
    if (instance->result != E_SUCCESS) {
        return;
    }

    VkResult err = { 0 };

    const char* deviceExt[1] = { "VK_KHR_swapchain" };
    const float queuePriorities[1] = { 1.f };

    VkDeviceQueueCreateInfo dqcis[1] = { (VkDeviceQueueCreateInfo){
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pQueuePriorities = queuePriorities,
      .queueCount = 1,
      .queueFamilyIndex = instance->graphicsQueueFamilyIndex,
    } };

    VkDeviceCreateInfo dci = (VkDeviceCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .enabledExtensionCount = sizeof(deviceExt) / sizeof(*deviceExt),
        .ppEnabledExtensionNames = deviceExt,
        .queueCreateInfoCount = sizeof(dqcis) / sizeof(*dqcis),
        .pQueueCreateInfos = dqcis,
    };

    err =
      vkCreateDevice(instance->physicalDevice, &dci, NULL, &instance->device);
    if (err != VK_SUCCESS) {
        instance->result = E_CREATE_DEVICE_FAILURE;
        return;
    }

    vkGetDeviceQueue(instance->device,
      instance->graphicsQueueFamilyIndex,
      0,
      &instance->queue);
}

static void CreateDescriptorPool(EInstance instance) {
    if (instance->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    };
    uint32_t maxSets = { 0 };
    for (int i = 0; i < sizeof(poolSizes) / sizeof(*poolSizes); ++i) {
        maxSets += poolSizes[i].descriptorCount;
    }
    VkDescriptorPoolCreateInfo dpci = (VkDescriptorPoolCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .pPoolSizes = poolSizes,
        .poolSizeCount = sizeof(poolSizes) / sizeof(*poolSizes),
        .maxSets = maxSets,
    };
    err = vkCreateDescriptorPool(
      instance->device, &dpci, NULL, &instance->descriptorPool);
    if (err != VK_SUCCESS) {
        instance->result = E_CREATE_DESCRIPTOR_POOL_FAILURE;
    }
}

#if E_ENABLE_ERROR_CALLBACK
// Debug callbacks
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageTypes,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData) {
    (void)pUserData;
    (void)messageTypes;
    (void)messageSeverity;
    (void)fprintf(
      stderr, "(Vulkan) validation layer: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks* pAllocator,
  VkDebugUtilsMessengerEXT* pDebugMessenger) {

    PFN_vkVoidFunction func =
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func) {
        return ((PFN_vkCreateDebugUtilsMessengerEXT)func)(
          instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
  VkDebugUtilsMessengerEXT debugMessenger,
  const VkAllocationCallbacks* pAllocator) {

    PFN_vkVoidFunction func =
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func) {
        ((PFN_vkDestroyDebugUtilsMessengerEXT)func)(
          instance, debugMessenger, pAllocator);
    }
}
#endif

static void CreateInstance(EInstance instance) {
    if (instance->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    const char** reqExt = { NULL };
    uint32_t reqExtCount = { 0 };
    reqExt = glfwGetRequiredInstanceExtensions(&reqExtCount);
    if (!reqExt) {
        instance->result = E_GLFW_FAILURE;
        return;
    }

    // additional required extensions
    const char* const addReqExt[] = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
#if E_ENABLE_ERROR_CALLBACK
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };
    const uint32_t addReqExtCount = { sizeof(addReqExt) / sizeof(*addReqExt) };

    VkInstanceCreateInfo ici = { 0 };
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    ici.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
#if E_ENABLE_ERROR_CALLBACK
    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    ici.enabledLayerCount = 1;
    ici.ppEnabledLayerNames = layers;
    // IsValidationLayerSupported()
    VkDebugUtilsMessengerCreateInfoEXT duimci =
      (VkDebugUtilsMessengerCreateInfoEXT){
          .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
          .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
          .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
          .pfnUserCallback = DebugUtilsCallback,
      };
    ici.pNext = &duimci;
#endif

    VkExtensionProperties* props = { NULL };
    uint32_t propCount = { 0 };
    err = vkEnumerateInstanceExtensionProperties(NULL, &propCount, NULL);
    if (err != VK_SUCCESS) {
        instance->result = E_ENUMERATE_FAILURE;
        return;
    }

    props = malloc(sizeof(*props) * propCount);
    // Worst case scenario malloc. Small sizes so doesn't really matter.
    instance->exts =
      malloc(sizeof(*instance->exts) * (propCount + addReqExtCount));
    if (!props || !instance->exts) {
        free(props);
        instance->result = E_MALLOC_FAILURE;
        return;
    }

    err = vkEnumerateInstanceExtensionProperties(NULL, &propCount, props);
    if (err != VK_SUCCESS) {
        instance->result = E_ENUMERATE_FAILURE;
        free(props);
        return;
    }

#if E_VERBOSE_MESSAGING
    printf("Available Vulkan extensions:\n");
    for (int i = 0; i < propCount; ++i) {
        printf("\t%s\n", props[i].extensionName);
    }
    printf("Required Vulkan extensions:\n");
    for (int i = 0; i < reqExtCount; ++i) {
        printf("\t%s\n", reqExt[i]);
    }
    for (int i = 0; i < addReqExtCount; ++i) {
        printf("\t%s\n", addReqExt[i]);
    }
#endif

    uint32_t addNext = { 0 };
    // add all required extensions to window->instanceExts
    for (int i = 0; i < propCount; ++i) {
        // searching through all of glfw's required extensions
        for (int j = 0; j < reqExtCount; ++j) {
            if (strcmp(props[i].extensionName, reqExt[j]) == 0) {
                instance->exts[addNext++] = props[i].extensionName;
            }
        }
        // searching through all of additional required extensions
        for (int j = 0; j < addReqExtCount; ++j) {
            if (strcmp(props[i].extensionName, addReqExt[j]) == 0) {
                instance->exts[addNext++] = props[i].extensionName;
            }
        }
    }
    instance->extsCount = addNext;

    ici.enabledExtensionCount = instance->extsCount;
    ici.ppEnabledExtensionNames = instance->exts;
    err = vkCreateInstance(&ici, NULL, &instance->instance);
    if (err != VK_SUCCESS) {
        instance->result = E_CREATE_INSTANCE_FAILURE;
        free(props);
        return;
    }
    free(props);

#if E_ENABLE_ERROR_CALLBACK
    err = CreateDebugUtilsMessengerEXT(
      instance->instance, &duimci, NULL, &instance->debugMessenger);
    if (err != VK_SUCCESS) {
        instance->result = E_FAILURE;
    }
#endif
}
