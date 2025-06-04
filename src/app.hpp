#pragma once
#include "../graphics.h"

struct AppCreateInfo {
    const char* title{ nullptr };
    struct {
        int width;
        int height;
    } size{};
};

class App {
public:
    explicit App(AppCreateInfo& info);
    ~App();

    App(const App&) = delete;
    App(App&&) noexcept = default;
    auto operator=(const App&) -> App& = delete;
    auto operator=(App&&) noexcept -> App& = default;

private:
    EWindow m_window{ nullptr };
    EContext m_context{ nullptr };
};
