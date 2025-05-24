#include "instance.h"
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
    uint32_t extsCount;
    uint32_t graphicsQueueFamilyIndex;
};

#if E_ENABLE_ERROR_CALLBACK
static VkBool32 IsValidationLayerSupported(void) {
    VkResult err = { 0 };
    uint32_t layerCount = { 0 };

    err = vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    if (err != VK_SUCCESS) {
        return 0;
    }
    VkLayerProperties* layers = malloc(sizeof(*layers) * layerCount);
    if (!layers) {
        return 0;
    }
    err = vkEnumerateInstanceLayerProperties(&layerCount, layers);
    if (err != VK_SUCCESS) {
        free(layers);
        return 0;
    }

#if E_VERBOSE_MESSAGING
    printf("Available layers:\n");
    for (int i = 0; i < layerCount; ++i) {
        printf("\t%s - %s\n", layers[i].layerName, layers[i].description);
    }
#endif
    for (int i = 0; i < layerCount; ++i) {
        if (strcmp("VK_LAYER_KHRONOS_validation", layers[i].layerName) == 0) {
            return 1;
        }
    }
    return 0;
}

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

// VkInstance initialization
E_EXTERN EInstance eCreateInstance(void) {
    EInstance res = malloc(sizeof(*res));
    if (!res) {
        return NULL;
    }
    *res = (struct EInstance_t){ 0 };

    VkResult err = { 0 };

    const char** reqExt = { NULL };
    uint32_t reqExtCount = { 0 };
    reqExt = glfwGetRequiredInstanceExtensions(&reqExtCount);
    if (!reqExt) {
        res->result = E_GLFW_FAILURE;
        return res;
    }

    // additional required extensions
    const char* const addReqExt[] = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
#ifdef E_ENABLE_ERROR_CALLBACK
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };
    const uint32_t addReqExtCount = { sizeof(addReqExt) / sizeof(*addReqExt) };

    VkInstanceCreateInfo ici = { 0 };
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    ici.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
#ifdef E_ENABLE_ERROR_CALLBACK
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
        res->result = E_ENUMERATE_FAILURE;
        return res;
    }

    props = malloc(sizeof(*props) * propCount);
    // Worst case scenario malloc. Small sizes so doesn't really matter.
    res->exts = malloc(sizeof(*res->exts) * (propCount + addReqExtCount));
    if (!props || !res->exts) {
        res->result = E_MALLOC_FAILURE;
        return res;
    }

    err = vkEnumerateInstanceExtensionProperties(NULL, &propCount, props);
    if (err != VK_SUCCESS) {
        res->result = E_ENUMERATE_FAILURE;
        return res;
    }

#if E_VERBOSE_MESSAGING
    printf("Available Vulkan extensions:\n");
    for (int i = 0; i < propCount; ++i) {
        printf("\t%s\n", props[i].extensionName);
    }
    printf("Required extensions:\n");
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
                res->exts[addNext++] = props[i].extensionName;
            }
        }
        // searching through all of additional required extensions
        for (int j = 0; j < addReqExtCount; ++j) {
            if (strcmp(props[i].extensionName, addReqExt[j]) == 0) {
                res->exts[addNext++] = props[i].extensionName;
            }
        }
    }
    res->extsCount = addNext;

    ici.enabledExtensionCount = res->extsCount;
    ici.ppEnabledExtensionNames = res->exts;
    err = vkCreateInstance(&ici, NULL, &res->instance);
    if (err != VK_SUCCESS) {
        res->result = E_CREATE_INSTANCE_FAILURE;
        return res;
    }

#if E_ENABLE_ERROR_CALLBACK
    err = CreateDebugUtilsMessengerEXT(
      res->instance, &duimci, NULL, &res->debugMessenger);
    if (err != VK_SUCCESS) {
        res->result = E_FAILURE;
        return res;
    }
#endif

    free(props);

    SelectPhysicalDevice(res);
    SelectGraphicsQueueFamilyIndex(res);

    return res;
}

// cleanup
E_EXTERN void eDestroyInstance(EInstance instance) {
    free(instance->exts);
#if E_ENABLE_ERROR_CALLBACK
    DestroyDebugUtilsMessengerEXT(
      instance->instance, instance->debugMessenger, NULL);
#endif
    free(instance);
    vkDestroyInstance(instance->instance, NULL);
}
