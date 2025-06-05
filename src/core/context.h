#pragma once

#include "../graphics.h"

E_EXTERN void eCreateContext(EContext* contextOut);
E_EXTERN void eDestroyContext(EContext context);
E_EXTERN void eWaitForQueues(EContext context);
