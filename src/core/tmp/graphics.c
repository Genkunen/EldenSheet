#include "../graphics.h"

EResult eGetResult(void* handleIn) {
    if (!handleIn) {
        return E_FAILURE;
    }
    return *(EResult*)handleIn;
}
