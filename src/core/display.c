#include "display.h"

#include "core.h"

#include <stdlib.h>

static void CreateSurface(EDisplay display, EWindow window, EContext context);
static void SelectSurfaceFormat(EDisplay display, EContext context);
static void SelectPresentMode(EDisplay display);
static void CreateSwapchain(EDisplay display, EContext context);

E_EXTERN void eCreateDisplay(EDisplay* displayOut, EDisplayCreateInfo* infoIn) {
    if (!displayOut) {
        return;
    }
    EDisplay display = malloc(sizeof(*display));
    if (!display) {
        *displayOut = NULL;
        return;
    }
    *displayOut = display;
    *display = (struct EDisplay_t){ 0 };

    if (!infoIn) {
        display->result = E_CREATE_INFO_MISSING;
        return;
    }
    if (!infoIn->context || !infoIn->window) {
        display->result = E_CREATE_INFO_MISSING_VALUE;
        return;
    }

    CreateSurface(display, infoIn->window, infoIn->context);
    SelectSurfaceFormat(display, infoIn->context);
    SelectPresentMode(display);
    CreateSwapchain(display, infoIn->context);

    display->result = 1;
}

E_EXTERN void eDestroyDisplay(EDisplay display, EContext context) {
    vkDestroySurfaceKHR(context->instance, display->surface, NULL);
    free(display);
    //
}

static void CreateSwapchain(EDisplay display, EContext context) {
    if (display->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };
    VkSwapchainKHR oldSwapchain = display->swapchain;
    display->swapchain = VK_NULL_HANDLE;
    err = vkDeviceWaitIdle(context->device);
    if (err != VK_SUCCESS) {
        context->result = E_SYNC_FAILURE;
        return;
    }
}

static void SelectPresentMode(EDisplay display) {
    if (display->result != E_SUCCESS) {
        return;
    }
    // FIFO is the only REQUIRED to exist present mode by Vulkan
    // and I think its irrelevant to use any other?
    display->presentMode = VK_PRESENT_MODE_FIFO_KHR;
}

static void SelectSurfaceFormat(EDisplay display, EContext context) {
    if (display->result != E_SUCCESS) {
        return;
    }

    VkResult err = { 0 };
    VkBool32 res = { 0 };

    err = vkGetPhysicalDeviceSurfaceSupportKHR(context->physicalDevice,
      context->graphicsQueueFamilyIndex,
      display->surface,
      &res);
    if (err != VK_SUCCESS || res != VK_TRUE) {
        context->result = E_NO_AVAILABLE_WSI_SUPPORT;
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
      context->physicalDevice, display->surface, &srfFmtCount, NULL);
    if (err != VK_SUCCESS) {
        context->result = E_ENUMERATE_FAILURE;
        return;
    }
    VkSurfaceFormatKHR* srfFmts = malloc(sizeof(*srfFmts) * srfFmtCount);
    if (!srfFmts) {
        context->result = E_MALLOC_FAILURE;
        return;
    }
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(
      context->physicalDevice, display->surface, &srfFmtCount, srfFmts);
    if (err != VK_SUCCESS) {
        context->result = E_ENUMERATE_FAILURE;
        goto return_early;
    }

    if (srfFmtCount == 1) {
        if (srfFmts[0].format == VK_FORMAT_UNDEFINED) {
            display->surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
            display->surfaceFormat.colorSpace =
              VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            goto return_early;
        }
        display->surfaceFormat = srfFmts[0];
        goto return_early;
    }

    VkSurfaceFormatKHR* fmt = { NULL };
    const VkFormat* reqFmt = { NULL };
    uint32_t reqFmtSize = sizeof(reqFmts) / sizeof(*reqFmts);

    // search if requested format is found
    for (fmt = srfFmts; fmt != srfFmts + srfFmtCount; ++fmt) {
        for (reqFmt = reqFmts; reqFmt != reqFmts + reqFmtSize; reqFmt++) {
            if (fmt->format == *reqFmt && fmt->colorSpace == reqColorSpace) {
                display->surfaceFormat = *fmt;
                goto return_early;
            }
        }
    }
    // if none found use whatever first available
    display->surfaceFormat = *srfFmts;
return_early:
    free(srfFmts);
}

static void CreateSurface(EDisplay display, EWindow window, EContext context) {
    if (display->result != E_SUCCESS) {
        return;
    }
    VkResult err = glfwCreateWindowSurface(
      context->instance, window->window, NULL, &display->surface);
    if (err != VK_SUCCESS) {
        context->result = E_GLFW_FAILURE;
    }
}
