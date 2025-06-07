#pragma once

#include "../graphics.h"

E_EXTERN void eCreateRenderer(ERenderer* rendererOut,
  ERendererCreateInfo* infoIn);
E_EXTERN void eDestroyRenderer(ERenderer renderer, EContext context);
