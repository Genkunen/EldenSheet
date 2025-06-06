#include "window.h"

#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if E_ENABLE_ERROR_CALLBACK
static void GlfwErrorCallback(int error, const char* description) {
    (void)fprintf(stderr, "(GLFW) Error: %d: %s\n", error, description);
}
#endif

static void InitWindow(EWindow window, EWindowCreateInfo* infoIn) {
    if (window->result != E_SUCCESS) {
        return;
    }
    if (!infoIn) {
        window->result = E_CREATE_INFO_MISSING;
        return;
    }
    if (!infoIn->title || !infoIn->size.width || !infoIn->size.height) {
        window->result = E_CREATE_INFO_MISSING_VALUE;
        return;
    }
    *window = (struct EWindow_t){
        .title = infoIn->title,
        .size = {
          .width = infoIn->size.width,
          .height = infoIn->size.height,
        },
    };

#if E_ENABLE_ERROR_CALLBACK
    (void)glfwSetErrorCallback(GlfwErrorCallback);
#endif

    if (!glfwInit()) {
        window->result = E_GLFW_FAILURE;
        return;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window->window = glfwCreateWindow(
      infoIn->size.width, infoIn->size.height, infoIn->title, NULL, NULL);
    if (!glfwVulkanSupported()) {
        window->result = E_GLFW_FAILURE;
    }
}

// GLFW initialization
E_EXTERN void eCreateWindow(EWindow* windowOut, EWindowCreateInfo* infoIn) {
    if (!windowOut) {
        return;
    }
    EWindow window = malloc(sizeof(*window));
    if (!window) {
        *windowOut = NULL;
        return;
    }
    *windowOut = window;
    *window = (struct EWindow_t){ 0 };

    InitWindow(window, infoIn);
}

// cleanup
E_EXTERN void eDestroyWindow(EWindow window) {
    glfwDestroyWindow(window->window);
    free(window);
    glfwTerminate();
}

E_EXTERN int eWindowShouldClose(EWindow window) {
    return glfwWindowShouldClose(window->window);
}

E_EXTERN void ePollEvents(void) {
    glfwPollEvents();
}

E_EXTERN int eWindowShouldResize(EWindow window) {
    int newWidth = { 0 };
    int newHeight = { 0 };
    glfwGetFramebufferSize(window->window, &newWidth, &newHeight);
    if (newWidth == 0 || newHeight == 0) {
        return 0;
    }
    return (window->size.width != newWidth || window->size.height != newHeight)
           || window->shouldResize;
}

E_EXTERN int eWindowIsMinimized(EWindow window) {
    return glfwGetWindowAttrib(window->window, GLFW_ICONIFIED);
}
