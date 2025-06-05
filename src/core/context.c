#include "context.h"

#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

static void SelectPhysicalDevice(EContext context);
static void SelectGraphicsQueueFamilyIndex(EContext context);
static void CreateLogicalDevice(EContext context);
static void CreateDescriptorPool(EContext context);
static void CreateInstance(EContext context);


// VkInstance initialization
E_EXTERN void eCreateContext(EContext* contextOut) {
    if (!contextOut) {
        return;
    }
    EContext context = malloc(sizeof(*context));
    if (!context) {
        *contextOut = NULL;
        return;
    }
    *contextOut = context;
    *context = (struct EContext_t){ 0 };

    CreateInstance(context);
    SelectPhysicalDevice(context);
    SelectGraphicsQueueFamilyIndex(context);
    CreateLogicalDevice(context);
    CreateDescriptorPool(context);
}

// cleanup
E_EXTERN void eDestroyContext(EContext context) {
    free(context->exts);
#if E_ENABLE_ERROR_CALLBACK
    DestroyDebugUtilsMessengerEXT(
      context->instance, context->debugMessenger, NULL);
#endif
    vkDestroyDescriptorPool(context->device, context->descriptorPool, NULL);
    vkDestroyDevice(context->device, NULL);
    vkDestroyInstance(context->instance, NULL);
    free(context);
}

static void SelectGraphicsQueueFamilyIndex(EContext context) {
    if (context->result != E_SUCCESS) {
        return;
    }
    uint32_t count = { 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(
      context->physicalDevice, &count, NULL);
    VkQueueFamilyProperties* props = malloc(sizeof(*props) * count);
    if (!props) {
        context->result = E_MALLOC_FAILURE;
        return;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(
      context->physicalDevice, &count, props);
    for (int i = 0; i < count; ++i) {
        if (props->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            context->graphicsQueueFamilyIndex = i;
            return;
        }
    }
    context->result = E_NO_AVAILABLE_GRAPHICS_QUEUES;
}

static void CreateLogicalDevice(EContext context) {
    if (context->result != E_SUCCESS) {
        return;
    }

    VkResult err = { 0 };

    const char* deviceExt[1] = { "VK_KHR_swapchain" };
    const float queuePriorities[1] = { 1.f };

    VkDeviceQueueCreateInfo dqcis[1] = { (VkDeviceQueueCreateInfo){
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pQueuePriorities = queuePriorities,
      .queueCount = 1,
      .queueFamilyIndex = context->graphicsQueueFamilyIndex,
    } };

    VkDeviceCreateInfo dci = (VkDeviceCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .enabledExtensionCount = sizeof(deviceExt) / sizeof(*deviceExt),
        .ppEnabledExtensionNames = deviceExt,
        .queueCreateInfoCount = sizeof(dqcis) / sizeof(*dqcis),
        .pQueueCreateInfos = dqcis,
    };

    err = vkCreateDevice(context->physicalDevice, &dci, NULL, &context->device);
    if (err != VK_SUCCESS) {
        context->result = E_CREATE_DEVICE_FAILURE;
        return;
    }

    vkGetDeviceQueue(
      context->device, context->graphicsQueueFamilyIndex, 0, &context->queue);
}

static void CreateDescriptorPool(EContext context) {
    if (context->result != E_SUCCESS) {
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
      context->device, &dpci, NULL, &context->descriptorPool);
    if (err != VK_SUCCESS) {
        context->result = E_CREATE_DESCRIPTOR_POOL_FAILURE;
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

static void CreateInstance(EContext context) {
    if (context->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    const char** reqExt = { NULL };
    uint32_t reqExtCount = { 0 };
    reqExt = glfwGetRequiredInstanceExtensions(&reqExtCount);
    if (!reqExt) {
        context->result = E_GLFW_FAILURE;
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
    const char* layers[1] = { "VK_LAYER_KHRONOS_validation" };
    ici.enabledLayerCount = 1;
    ici.ppEnabledLayerNames = layers;
    // IsValidationLayerSupported()
    VkDebugUtilsMessengerCreateInfoEXT duimci =
      (VkDebugUtilsMessengerCreateInfoEXT){
          .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
          .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
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
        context->result = E_ENUMERATE_FAILURE;
        return;
    }

    props = malloc(sizeof(*props) * propCount);
    // Worst case scenario malloc. Small sizes so doesn't really matter.
    context->exts =
      malloc(sizeof(*context->exts) * (propCount + addReqExtCount));
    if (!props || !context->exts) {
        free(props);
        context->result = E_MALLOC_FAILURE;
        return;
    }

    err = vkEnumerateInstanceExtensionProperties(NULL, &propCount, props);
    if (err != VK_SUCCESS) {
        context->result = E_ENUMERATE_FAILURE;
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
                context->exts[addNext++] = props[i].extensionName;
            }
        }
        // searching through all of additional required extensions
        for (int j = 0; j < addReqExtCount; ++j) {
            if (strcmp(props[i].extensionName, addReqExt[j]) == 0) {
                context->exts[addNext++] = props[i].extensionName;
            }
        }
    }
    context->extsCount = addNext;

    ici.enabledExtensionCount = context->extsCount;
    ici.ppEnabledExtensionNames = context->exts;
    err = vkCreateInstance(&ici, NULL, &context->instance);
    if (err != VK_SUCCESS) {
        context->result = E_CREATE_INSTANCE_FAILURE;
        free(props);
        return;
    }
    free(props);

#if E_ENABLE_ERROR_CALLBACK
    err = CreateDebugUtilsMessengerEXT(
      context->instance, &duimci, NULL, &context->debugMessenger);
    if (err != VK_SUCCESS) {
        context->result = E_FAILURE;
    }
#endif
}

static void SelectPhysicalDevice(EContext context) {
    if (context->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    uint32_t deviceCount = { 0 };
    err = vkEnumeratePhysicalDevices(context->instance, &deviceCount, NULL);
    if (err != VK_SUCCESS) {
        context->result = E_ENUMERATE_FAILURE;
        return;
    }
    VkPhysicalDevice* devices = malloc(sizeof(*devices) * deviceCount);
    if (!devices) {
        context->result = E_MALLOC_FAILURE;
        return;
    }
    err = vkEnumeratePhysicalDevices(context->instance, &deviceCount, devices);
    if (err != VK_SUCCESS) {
        context->result = E_ENUMERATE_FAILURE;
        free(devices);
        return;
    }
    VkPhysicalDeviceProperties prop = { 0 };
    for (int i = 0; i < deviceCount; ++i) {
        vkGetPhysicalDeviceProperties(devices[i], &prop);
        if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            context->physicalDevice = devices[i];
            break;
        }
    }
    if (deviceCount > 0) {
        context->physicalDevice = devices[0];
    }
    else {
        context->result = E_NO_AVAILABLE_PHYSICAL_DEVICES;
    }
    free(devices);
}
