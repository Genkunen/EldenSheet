#include "app.hpp"

#include "context.h"
#include "display.h"
#include "window.h"


#include <iostream>
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

    EDisplayCreateInfo dci{};
    dci.context = m_context;
    dci.window = m_window;
    eCreateDisplay(&m_display, &dci);
    Check(m_display);

    while (!static_cast<bool>(eWindowShouldClose(m_window))) {
        break;
    }
}

App::~App() {
    eDestroyDisplay(m_display, m_context);
    eDestroyContext(m_context);
    eDestroyWindow(m_window);
}
