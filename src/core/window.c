#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct EWindow_t {
    EResult result;
    GLFWwindow* window;
    struct {
        int width;
        int height;
    } size;
    const char* title;
};

#if E_ENABLE_ERROR_CALLBACK
static void GlfwErrorCallback(int error, const char* description) {
    (void)fprintf(stderr, "(GLFW) Error: %d: %s\n", error, description);
}
#endif

// GLFW initialization
EWindow eCreateWindow(EWindowCreateInfo* infoIn) {
    EWindow res = malloc(sizeof(*res));
    if (!res) {
        return NULL;
    }
    *res = (struct EWindow_t){ 0 };

    if (!infoIn) {
        res->result = E_CREATE_INFO_MISSING;
        return res;
    }
    if (!infoIn->title || !infoIn->size.width || !infoIn->size.height) {
        res->result = E_CREATE_INFO_MISSING_VALUE;
        return res;
    }

#if E_ENABLE_ERROR_CALLBACK
    (void)glfwSetErrorCallback(GlfwErrorCallback);
#endif

    if (!glfwInit()) {
        res->result = E_GLFW_FAILURE;
        return res;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    res->window = glfwCreateWindow(
      infoIn->size.width, infoIn->size.height, infoIn->title, NULL, NULL);
    if (!glfwVulkanSupported()) {
        res->result = E_GLFW_FAILURE;
        return res;
    }
    return res;
}

// cleanup
void eDestroyWindow(EWindow window) {
    glfwDestroyWindow(window->window);
    free(window);
    glfwTerminate();
}
