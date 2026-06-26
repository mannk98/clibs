/*
 * PID.h
 *
 *  Created on: Apr 26, 2022
 *      Author: mannk
 */

#ifndef PID_H_
#define PID_H_

typedef struct {

	/* Controller gains */
	float Kp;
	float Ki;
	float Kd;

	/* Derivative low-pass filter time constant */
	float tau;

	/* Output limits */
	float limMin;
	float limMax;

	/* Integrator limits */
	float limMinInt;
	float limMaxInt;

	/* Sample time (in seconds) */
	float T;

	/* Controller "memory" */
	float integrator;
	float prevError; /* Required for integrator */
	float differentiator;
	float prevMeasurement; /* Required for differentiator */

	/* Controller output */
	float out;

} PIDController;

void PIDController_Init(volatile PIDController *pid);
// measurement is real point;
float PIDController_Update(volatile PIDController *pid, float setpoint,
		float measurement);

#endif /* PID_H_ */
