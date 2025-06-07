#include "imgui_layer.hpp"

#include "core.h"
#include "renderer.h"


#include <array>
#include <imgui_impl_glfw.h>
#include <memory>
#include <string>


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

    auto offsets = std::make_unique<std::array<uint32_t, 3>>();
    (*offsets)[0] = offsetof(ImDrawVert, pos);
    (*offsets)[1] = offsetof(ImDrawVert, uv);
    (*offsets)[2] = offsetof(ImDrawVert, col);

    ERendererCreateInfo rci = {};
    rci.context = context;
    rci.display = display;
    rci.imguiVertData.inputAttrCount = 3;
    rci.imguiVertData.inputAttrSize = sizeof(ImDrawVert);
    rci.imguiVertData.inputAttrOffsets = offsets->data();

    eCreateRenderer(&renderer, &rci);
    if (renderer->result != E_SUCCESS) {
        throw std::exception(std::to_string(renderer->result).c_str());
    }
}

void eEndImgui(EContext context) noexcept {
    eDestroyRenderer(renderer, context);
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


void eDrawImgui(EDisplay display, EContext context, EWindow window) {
    // eBeginFrame();
    // ImGui_ImplGlfw_NewFrame();
    VkResult err{};


    ImGui::NewFrame();

    // app

    ImGui::Render();
    // eEndFrame();
}