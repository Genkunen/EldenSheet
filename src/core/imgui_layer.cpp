#include "imgui_layer.hpp"

#include "core.h"
#include "renderer.h"

#include <imgui_impl_glfw.h>

namespace {
ERenderer renderer = nullptr;
}

void eBeginImgui(EDisplay display, EContext context, EWindow window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    // initialize imgui
    ImGui_ImplGlfw_InitForVulkan(window->window, E_ENABLE_ERROR_CALLBACK);

    // initialize vulkan
    // io.BackendRendererUserData
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    eCreateRenderer(&renderer, context);

    // test
    {
        using namespace ImGui;
        ;
    }
}

void eEndImgui(EContext context) noexcept {
    eDestroyRenderer(renderer, context);
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
