/* Re-export forwarder: the CRC implementation lives once in common/crc.
 *
 * This translation unit compiles common/crc/crc.c into the esp-libs component,
 * so crc/crc.o (referenced by component.mk and the host Makefile targets) is
 * produced unchanged while the algorithm has a single source of truth and is
 * no longer duplicated here. The relative include resolves to clibs/common/crc/
 * (independent of -I flags). */
#include "../../common/crc/crc.c"
