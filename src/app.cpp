#include "app.hpp"

#include "context.h"
#include "window.h"

#include <iostream>
#include <string>

App::App(AppCreateInfo& info) {
    EWindowCreateInfo wci{};
    wci.title = info.title;
    wci.size = { info.size.width, info.size.height };
    eCreateWindow(&m_window, &wci);
    eCreateContext(&m_context, m_window);
    if (eGetResult(m_context) != E_SUCCESS) {
        std::cerr << "Error:\n"
                  << "Window: " << eGetResult(m_window) << "\n"
                  << "Instance: " << eGetResult(m_context) << "\n";
        throw std::runtime_error(std::to_string(eGetResult(m_context)));
    }
}

App::~App() {
    eDestroyContext(m_context);
    eDestroyWindow(m_window);
}
