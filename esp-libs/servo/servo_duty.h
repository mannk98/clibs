#ifndef CLIBS_SERVO_DUTY_H
#define CLIBS_SERVO_DUTY_H

#include <stdint.h>

/* Map angle (clamped to 0..180) to a servo pulse width in microseconds (1000..2000).
 * Pure: no SDK headers (host-testable). */
uint32_t servo_angle_to_duty(uint8_t angle);

#endif /* CLIBS_SERVO_DUTY_H */
