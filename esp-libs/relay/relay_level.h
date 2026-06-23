#ifndef CLIBS_RELAY_LEVEL_H
#define CLIBS_RELAY_LEVEL_H

#include <stdbool.h>

/* GPIO level (0/1) to drive for the desired on-state; active_low inverts.
 * Pure: no SDK headers (host-testable). */
int relay_level(bool on, bool active_low);

#endif /* CLIBS_RELAY_LEVEL_H */
