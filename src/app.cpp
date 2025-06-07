#include "app.hpp"

#include "context.h"
#include "display.h"
#include "window.h"

#include "imgui_layer.hpp"

#include <iostream>

#include <string>


App::App(AppCreateInfo& info) {
    (void)std::cout;
    auto Check = [](void* any) {
        if (eGetResult(any) != E_SUCCESS) {
            throw std::exception(std::to_string(eGetResult(any)).c_str());
        }
    };

    EWindowCreateInfo wci{};
    wci.title = info.title;
    wci.size = { info.size.width, info.size.height };
    eCreateWindow(&m_window, &wci);
    Check(m_window);

    eCreateContext(&m_context);
    Check(m_context);

    eCreateDisplay(&m_display, m_context, m_window);
    Check(m_display);

    eBeginImgui(m_display, m_context, m_window);

    while (!static_cast<bool>(eWindowShouldClose(m_window))) {
        ePollEvents();
        if (static_cast<bool>(eWindowShouldResize(m_window))) {
            eResizeWindow(m_display, m_context, m_window);
            Check(m_display);
        }
        if (static_cast<bool>(eWindowIsMinimized(m_window))) {
            continue;
        }

        eDrawImgui(m_display, m_context, m_window);

        if (!static_cast<bool>(eWindowShouldResize(m_window))) {
            eRenderFrame(m_display, m_context, m_window);
            Check(m_display);
        }

        if (!static_cast<bool>(eWindowShouldResize(m_window))) {
            eDisplayFrame(m_display, m_context);
            Check(m_display);
        }
    }
}

App::~App() {
    eEndImgui(m_context);
    eWaitForQueues(m_context);
    eDestroyDisplay(m_display, m_context);
    eDestroyContext(m_context);
    eDestroyWindow(m_window);
}
