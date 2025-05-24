#include "core.h"

EResult eGetResult(void* handleIn) {
    if (!handleIn) {
        return E_FAILURE;
    }
    return *(E_RESULT*)handleIn;
}
