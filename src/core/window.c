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
void eCreateWindow(EWindow* windowOut, EWindowCreateInfo* infoIn) {
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
void eDestroyWindow(EWindow window) {
    glfwDestroyWindow(window->window);
    free(window);
    glfwTerminate();
}

void* eGetGlfwWindow(EWindow window) {
    return window->window;
}
