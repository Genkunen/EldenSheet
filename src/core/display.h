#pragma once

#include "../graphics.h"

E_EXTERN void
  eCreateDisplay(EDisplay* displayOut, EContext context, EWindow window);
E_EXTERN void eDestroyDisplay(EDisplay display, EContext context);
E_EXTERN void eRenderFrame(EDisplay display, EContext context, EWindow window);
E_EXTERN void eDisplayFrame(EDisplay display, EContext context);
