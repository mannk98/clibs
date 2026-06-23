#include "relay_level.h"

int relay_level(bool on, bool active_low)
{
    if (active_low) {
        return on ? 0 : 1;
    }
    return on ? 1 : 0;
}
