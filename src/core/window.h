#pragma once

#include "core.h"

E_EXTERN EWindow eCreateWindow(EWindowCreateInfo* infoIn);
E_EXTERN void eDestroyWindow(EWindow window);
E_EXTERN void* eGetGlfwWindow(EWindow window);
