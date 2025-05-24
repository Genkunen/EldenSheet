#include "app.hpp"
#include <string>

auto main() -> int {
    AppCreateInfo aci{};
    aci.title = "Tymek";
    aci.size = { 1280, 720 };
    try {
        App app(aci);
    } catch (std::exception& err) {
        return std::stoi(err.what());
    }
    return 0;
}