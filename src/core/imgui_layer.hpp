#pragma once

extern "C" {
#include "../graphics.h"
}

void eBeginImgui(EDisplay display, EContext context, EWindow window);
void eDrawImgui(EDisplay display, EContext context, EWindow window);
void eEndImgui(EContext context) noexcept;
