#pragma once
#include "../../config.h"
#include <stdint.h>

#ifdef __cplusplus
#define E_EXTERN extern "C"
#else
#define E_EXTERN
#endif

#define E_OPAQUE_HANDLE(name) typedef struct name##_t* name

typedef enum EResult {
    E_SUCCESS = 0,

    E_FAILURE,
    E_MALLOC_FAILURE,
    E_GLFW_FAILURE,
    E_SYNC_FAILURE,
    E_ENUMERATE_FAILURE,
    E_CREATE_INSTANCE_FAILURE,
    E_CREATE_DEVICE_FAILURE,
    E_CREATE_DESCRIPTOR_POOL_FAILURE,
    E_CREATE_SWAPCHAIN_FAILURE,
    E_CREATE_RENDER_PASS_FAILURE,
    E_CREATE_IMAGE_VIEW_FAILURE,
    E_CREATE_FRAMEBUFFER_FAILURE,
    E_CREATE_COMMAND_POOL_FAILURE,
    E_CREATE_COMMAND_BUFFER_FAILURE,
    E_CREATE_FENCE_FAILURE,
    E_CREATE_SEMAPHORE_FAILURE,

    E_CREATE_INFO_MISSING,
    E_CREATE_INFO_MISSING_VALUE,

    E_NO_AVAILABLE_PHYSICAL_DEVICES,
    E_NO_AVAILABLE_GRAPHICS_QUEUES,
    E_NO_AVAILABLE_WSI_SUPPORT,

} EResult;

E_EXTERN EResult eGetResult(void* handleIn);

E_OPAQUE_HANDLE(EContext);
E_OPAQUE_HANDLE(EWindow);
E_OPAQUE_HANDLE(EDisplay);

typedef struct EWindowCreateInfo {
    const char* title;
    struct {
        int width;
        int height;
    } size;
} EWindowCreateInfo;

typedef struct EDisplayCreateInfo {
    EContext context;
    EWindow window;
} EDisplayCreateInfo;
