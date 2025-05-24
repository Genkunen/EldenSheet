#pragma once
#include "../../config.h"
#include <stdint.h>

#ifdef __cplusplus
#define E_EXTERN extern "C"
#else
#define E_EXTERN
#endif

#define E_OPAQUE_HANDLE(name) typedef struct name##_t* name

typedef enum E_RESULT {
    E_SUCCESS = 0,

    E_FAILURE,
    E_MALLOC_FAILURE,
    E_GLFW_FAILURE,
    E_ENUMERATE_FAILURE,
    E_CREATE_INSTANCE_FAILURE,

    E_CREATE_INFO_MISSING,
    E_CREATE_INFO_MISSING_VALUE,

    E_NO_AVAILABLE_PHYSICAL_DEVICES,
    E_NO_AVAILABLE_GRAPHICS_QUEUES,

} E_RESULT;

typedef E_RESULT EResult;

E_EXTERN EResult eGetResult(void* handleIn);

struct EWindowCreateInfo;

typedef struct EWindowCreateInfo {
    const char* title;
    struct {
        int width;
        int height;
    } size;
} EWindowCreateInfo;

E_OPAQUE_HANDLE(EInstance);
E_OPAQUE_HANDLE(EWindow);
