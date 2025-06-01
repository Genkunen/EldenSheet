#pragma once

#include "core.h"

E_EXTERN void eCreateWindow(EWindow* windowOut, EWindowCreateInfo* infoIn);
E_EXTERN void eDestroyWindow(EWindow window);
E_EXTERN void* eGetGlfwWindow(EWindow window);
