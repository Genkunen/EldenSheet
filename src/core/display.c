#include "display.h"

#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void CleanFrames(EDisplay display, EContext context);
static void CreateSurface(EDisplay display, EWindow window, EContext context);
static void SelectSurfaceFormat(EDisplay display, EContext context);
static void SelectPresentMode(EDisplay display);
static void CreateSwapchain(EDisplay display, EContext context);
static void CreateRenderPass(EDisplay display, EContext context);
static void CreateImageViews(EDisplay display, EContext context);
static void CreateFrameBuffer(EDisplay display, EContext context);
static void CreateCommandBuffer(EDisplay display, EContext context);

E_EXTERN void
  eCreateDisplay(EDisplay* displayOut, EContext context, EWindow window) {
    if (!displayOut || !context || !window) {
        return;
    }
    EDisplay display = malloc(sizeof(*display));
    if (!display) {
        *displayOut = NULL;
        return;
    }
    *displayOut = display;
    *display = (struct EDisplay_t){ 0 };

    glfwGetFramebufferSize(window->window, &display->width, &display->height);

    CreateSurface(display, window, context);
    SelectSurfaceFormat(display, context);
    SelectPresentMode(display);
    CreateSwapchain(display, context);
    CreateRenderPass(display, context);
    CreateImageViews(display, context);
    CreateFrameBuffer(display, context);
    CreateCommandBuffer(display, context);
}

E_EXTERN void eDestroyDisplay(EDisplay display, EContext context) {
    struct EFrame* curF = { NULL };
    while (display->frameCount--) {
        curF = &display->frames[display->frameCount];
        vkDestroyFramebuffer(context->device, curF->frameBuffer, NULL);
        vkDestroyImageView(context->device, curF->imageView, NULL);
        vkDestroyFence(context->device, curF->fence, NULL);
        vkDestroyCommandPool(context->device, curF->commandPool, NULL);
    }
    struct EFrameSemaphores* curS = { NULL };
    while (display->semaphoreCount--) {
        curS = &display->semaphores[display->semaphoreCount];
        vkDestroySemaphore(context->device, curS->imageAvailable, NULL);
        vkDestroySemaphore(context->device, curS->renderFinished, NULL);
    }

    vkDestroyRenderPass(context->device, display->renderPass, NULL);
    vkDestroySwapchainKHR(context->device, display->swapchain, NULL);
    vkDestroySurfaceKHR(context->instance, display->surface, NULL);
    free(display);
}

E_EXTERN void eDisplayFrame(EDisplay display, EContext context) {
    if (display->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    VkSemaphore renderFinished =
      display->semaphores[display->semaphoreCurrentIndex].renderFinished;

    VkPresentInfoKHR pi = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pImageIndices = &display->frameCurrentIndex,
        .pSwapchains = &display->swapchain,
        .swapchainCount = 1,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinished,
    };
    err = vkQueuePresentKHR(context->queue, &pi);
    if (err != VK_SUCCESS) {
        display->result = E_FRAME_DISPLAY_ERROR;
        return;
    }
    display->semaphoreCurrentIndex =
      (display->semaphoreCurrentIndex + 1) % display->semaphoreCount;
}

E_EXTERN void eRenderFrame(EDisplay display, EContext context, EWindow window) {
    if (display->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    struct EFrameSemaphores* curS =
      &display->semaphores[display->semaphoreCurrentIndex];

    VkSemaphore imgAvailable = curS->imageAvailable;
    VkSemaphore renderFinished = curS->renderFinished;

    err = vkAcquireNextImageKHR(context->device,
      display->swapchain,
      UINT32_MAX,
      imgAvailable,
      VK_NULL_HANDLE,
      &display->frameCurrentIndex);

    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
        window->shouldResize = 1;
        if (err == VK_ERROR_OUT_OF_DATE_KHR) {
            return;
        }
    }
    else if (err != VK_SUCCESS) {
        display->result = E_FRAME_RENDER_ERROR;
        return;
    }

    struct EFrame* curF = &display->frames[display->frameCurrentIndex];

    err = vkWaitForFences(context->device, 1, &curF->fence, 1, UINT32_MAX);
    if (err != VK_SUCCESS) {
        display->result = E_FRAME_RENDER_ERROR;
    }
    err = vkResetFences(context->device, 1, &curF->fence);
    if (err != VK_SUCCESS) {
        display->result = E_FRAME_RENDER_ERROR;
    }

    err = vkResetCommandPool(context->device, curF->commandPool, 0);
    if (err != VK_SUCCESS) {
        display->result = E_FRAME_RENDER_ERROR;
    }
    VkCommandBufferBeginInfo cbbi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    err = vkBeginCommandBuffer(curF->commandBuffer, &cbbi);
    if (err != VK_SUCCESS) {
        display->result = E_FRAME_RENDER_ERROR;
    }

    VkRenderPassBeginInfo rpbi = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = display->renderPass,
        .renderArea.extent = { 
            .width = display->width,
            .height = display->height, 
        },
        .clearValueCount = 1,
        .pClearValues = &display->clearValue,
        .framebuffer = curF->frameBuffer,
    };
    vkCmdBeginRenderPass(
      curF->commandBuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(curF->commandBuffer);
    VkPipelineStageFlags psf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &curF->commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinished,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imgAvailable,
        .pWaitDstStageMask = &psf,
    };
    err = vkEndCommandBuffer(curF->commandBuffer);
    if (err != VK_SUCCESS) {
        display->result = E_FRAME_RENDER_ERROR;
    }

    err = vkQueueSubmit(context->queue, 1, &si, curF->fence);
    if (err != VK_SUCCESS) {
        display->result = E_FRAME_RENDER_ERROR;
    }
}

static void CreateCommandBuffer(EDisplay display, EContext context) {
    if (display->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    uint32_t count = { display->frameCount };
    struct EFrame* curF = { NULL };
    while (count--) {
        curF = &display->frames[count];

        VkCommandPoolCreateInfo cpci = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = context->graphicsQueueFamilyIndex,
        };
        err =
          vkCreateCommandPool(context->device, &cpci, NULL, &curF->commandPool);
        if (err != VK_SUCCESS) {
            display->result = E_CREATE_COMMAND_POOL_FAILURE;
            return;
        }

        VkCommandBufferAllocateInfo cbai = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandBufferCount = 1,
            .commandPool = curF->commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        };
        err = vkAllocateCommandBuffers(
          context->device, &cbai, &curF->commandBuffer);
        if (err != VK_SUCCESS) {
            display->result = E_CREATE_COMMAND_BUFFER_FAILURE;
            return;
        }

        VkFenceCreateInfo fci = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        err = vkCreateFence(context->device, &fci, NULL, &curF->fence);
        if (err != VK_SUCCESS) {
            display->result = E_CREATE_FENCE_FAILURE;
            return;
        }
    }

    count = display->semaphoreCount;
    struct EFrameSemaphores* curS = { NULL };
    while (count--) {
        curS = &display->semaphores[count];

        VkSemaphoreCreateInfo sci = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        err =
          vkCreateSemaphore(context->device, &sci, NULL, &curS->imageAvailable);
        if (err != VK_SUCCESS) {
            display->result = E_CREATE_SEMAPHORE_FAILURE;
            return;
        }

        err =
          vkCreateSemaphore(context->device, &sci, NULL, &curS->renderFinished);
        if (err != VK_SUCCESS) {
            display->result = E_CREATE_SEMAPHORE_FAILURE;
            return;
        }
    }
}

static void CreateFrameBuffer(EDisplay display, EContext context) {
    if (display->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    VkFramebufferCreateInfo fci = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .attachmentCount = 1,
        .width = display->width,
        .height = display->height,
        .layers = 1,
        .renderPass = display->renderPass,
    };
    uint32_t count = { display->frameCount };
    struct EFrame* curr = { NULL };
    while (count--) {
        curr = &display->frames[count];
        fci.pAttachments = &curr->imageView;
        err =
          vkCreateFramebuffer(context->device, &fci, NULL, &curr->frameBuffer);
        if (err != VK_SUCCESS) {
            display->result = E_CREATE_FRAMEBUFFER_FAILURE;
            return;
        }
    }
}

static void CreateImageViews(EDisplay display, EContext context) {
    if (display->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    VkImageSubresourceRange imgRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    VkImageViewCreateInfo ivci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .format = display->surfaceFormat.format,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .components = { 
          .a = VK_COMPONENT_SWIZZLE_A, 
          .r = VK_COMPONENT_SWIZZLE_R,
          .g = VK_COMPONENT_SWIZZLE_G,
          .b = VK_COMPONENT_SWIZZLE_B, 
        },
        .subresourceRange = imgRange,
    };

    uint32_t count = { display->frameCount };
    struct EFrame* curr = { NULL };
    while (count--) {
        curr = &display->frames[count];
        ivci.image = curr->image;
        err = vkCreateImageView(context->device, &ivci, NULL, &curr->imageView);
        if (err != VK_SUCCESS) {
            display->result = E_CREATE_IMAGE_VIEW_FAILURE;
            return;
        }
    }
}


static void CreateRenderPass(EDisplay display, EContext context) {
    if (display->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    VkAttachmentDescription attDesc = {
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .format = display->surfaceFormat.format,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .samples = VK_SAMPLE_COUNT_1_BIT,
    };
    VkAttachmentReference attRef = {
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .attachment = 0,
    };
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pColorAttachments = &attRef,
        .colorAttachmentCount = 1,
    };
    VkSubpassDependency dep = {
        .dstSubpass = 0,
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkRenderPassCreateInfo rpci = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pSubpasses = &subpass,
        .subpassCount = 1,
        .pAttachments = &attDesc,
        .attachmentCount = 1,
        .pDependencies = &dep,
        .dependencyCount = 1,
    };
    err =
      vkCreateRenderPass(context->device, &rpci, NULL, &display->renderPass);
    if (err != VK_SUCCESS) {
        display->result = E_CREATE_RENDER_PASS_FAILURE;
    }
}

static void DestroyFrames(EDisplay display, EContext context) {
    uint32_t count = display->frameCount;
    struct EFrame* curr = { 0 };
    while (count--) {
        curr = &display->frames[count];

        vkDestroyFence(context->device, curr->fence, NULL);
        curr->fence = VK_NULL_HANDLE;

        vkFreeCommandBuffers(
          context->device, curr->commandPool, 1, &curr->commandBuffer);
        curr->commandBuffer = VK_NULL_HANDLE;

        vkDestroyCommandPool(context->device, curr->commandPool, NULL);
        curr->commandPool = VK_NULL_HANDLE;

        vkDestroyImageView(context->device, curr->imageView, NULL);
        vkDestroyFramebuffer(context->device, curr->frameBuffer, NULL);
    }
}

static void DestroySemaphores(EDisplay display, EContext context) {
    uint32_t count = { display->semaphoreCount };
    struct EFrameSemaphores* curr = { NULL };
    while (count--) {
        curr = &display->semaphores[count];
        vkDestroySemaphore(context->device, curr->imageAvailable, NULL);
        curr->imageAvailable = NULL;
        vkDestroySemaphore(context->device, curr->renderFinished, NULL);
        curr->renderFinished = NULL;
    }
}

static void CleanFrames(EDisplay display, EContext context) {
    DestroyFrames(display, context);
    DestroySemaphores(display, context);
    free(display->frames);
    free(display->semaphores);
    display->frameCount = 0;
    if (display->renderPass) {
        vkDestroyRenderPass(context->device, display->renderPass, NULL);
    }
}

static void CreateSwapchain(EDisplay display, EContext context) {
    if (display->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    VkSwapchainKHR oldSwapchain = { display->swapchain };
    display->swapchain = VK_NULL_HANDLE;
    err = vkDeviceWaitIdle(context->device);
    if (err != VK_SUCCESS) {
        context->result = E_SYNC_FAILURE;
        return;
    }

    CleanFrames(display, context);

    VkSurfaceCapabilitiesKHR cap = { 0 };
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      context->physicalDevice, display->surface, &cap);
    if (err != VK_SUCCESS) {
        display->result = E_CREATE_SWAPCHAIN_FAILURE;
        return;
    }


    uint32_t minImageCount = min(
      cap.maxImageCount ? cap.maxImageCount : 999, max(2, cap.minImageCount));
    VkSurfaceTransformFlagBitsKHR preTransform =
      (cap.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        : cap.currentTransform;

    VkSwapchainCreateInfoKHR sci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .oldSwapchain = oldSwapchain,
        .clipped = VK_TRUE,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .imageArrayLayers = 1,
        .imageColorSpace = display->surfaceFormat.colorSpace,
        .imageFormat = display->surfaceFormat.format,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .minImageCount = minImageCount,  // 2 for FIFO
        .preTransform = preTransform,
        .presentMode = display->presentMode,
        .surface = display->surface,
        .imageExtent = { .width = display->width, .height = display->height },
    };
    err =
      vkCreateSwapchainKHR(context->device, &sci, NULL, &display->swapchain);
    if (err != VK_SUCCESS) {
        display->result = E_CREATE_SWAPCHAIN_FAILURE;
        return;
    }
    err = vkGetSwapchainImagesKHR(
      context->device, display->swapchain, &display->frameCount, NULL);
    if (err != VK_SUCCESS) {
        display->result = E_CREATE_SWAPCHAIN_FAILURE;
        return;
    }

    VkImage images[8] = { 0 };
    if (display->frameCount > 8) {
        display->result = E_CREATE_SWAPCHAIN_FAILURE;
        return;
    }
    err = vkGetSwapchainImagesKHR(
      context->device, display->swapchain, &display->frameCount, images);
    if (err != VK_SUCCESS) {
        display->result = E_CREATE_SWAPCHAIN_FAILURE;
        return;
    }
    display->semaphoreCount = display->frameCount + 1;
    display->frames = calloc(display->frameCount, sizeof(*display->frames));
    display->semaphores =
      calloc(display->semaphoreCount, sizeof(*display->semaphores));
    if (!display->frames || !display->semaphores) {
        display->result = E_CREATE_SWAPCHAIN_FAILURE;
        free(display->frames);
        free(display->semaphores);
        return;
    }
    for (uint32_t i = 0; i < display->frameCount; ++i) {
        display->frames[i].image = images[i];
    }
    if (oldSwapchain) {
        vkDestroySwapchainKHR(context->device, display->swapchain, NULL);
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
    VkSurfaceFormatKHR* srfFmts = calloc(srfFmtCount, sizeof(*srfFmts));
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
    uint32_t reqFmtSize = { sizeof(reqFmts) / sizeof(*reqFmts) };

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
