#include "app.hpp"

#include "context.h"
#include "display.h"
#include "window.h"

#include <string>

App::App(AppCreateInfo& info) {
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

    while (!static_cast<bool>(eWindowShouldClose(m_window))) {
        ePollEvents();
        if (static_cast<bool>(eWindowShouldResize(m_window))) {
            // handle resizing
        }
        if (static_cast<bool>(eWindowIsMinimized(m_window))) {
            continue;
        }

        eRenderFrame(m_display, m_context, m_window);

        eDisplayFrame(m_display, m_context);
    }
}

App::~App() {
    eWaitForQueues(m_context);
    eDestroyDisplay(m_display, m_context);
    eDestroyContext(m_context);
    eDestroyWindow(m_window);
}
