#include "app.hpp"

#include "instance.h"
#include "window.h"

#include <iostream>
#include <string>

App::App(AppCreateInfo& info) {
    EWindowCreateInfo wci{};
    wci.title = info.title;
    wci.size = { info.size.width, info.size.height };
    m_window = eCreateWindow(&wci);
    eCreateInstance(&m_instance);
    if (eGetResult(m_instance) != E_SUCCESS) {
        std::cerr << "Error:\n"
                  << "Window: " << eGetResult(m_window) << "\n"
                  << "Instance: " << eGetResult(m_instance) << "\n";
        throw std::runtime_error(std::to_string(eGetResult(m_instance)));
    }
}

App::~App() {
    eDestroyInstance(m_instance);
    eDestroyWindow(m_window);
}
