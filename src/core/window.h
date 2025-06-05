#pragma once

#include "../graphics.h"

E_EXTERN void eCreateWindow(EWindow* windowOut, EWindowCreateInfo* infoIn);
E_EXTERN void eDestroyWindow(EWindow window);
E_EXTERN int eWindowShouldClose(EWindow window);
