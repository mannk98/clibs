#include "unity.h"
#include "PID.h"

void setUp(void) {}
void tearDown(void) {}

/* helper: a PID with wide limits unless a test overrides */
static void base_pid(PIDController *p) {
    p->Kp = 0; p->Ki = 0; p->Kd = 0;
    p->tau = 0.02f; p->T = 0.1f;
    p->limMin = -1e6f; p->limMax = 1e6f;
    p->limMinInt = -1e6f; p->limMaxInt = 1e6f;
    PIDController_Init(p);
}

static void test_proportional_only(void) {
    PIDController p; base_pid(&p);
    p.Kp = 2.0f;
    float out = PIDController_Update(&p, 10.0f, 0.0f); /* error = 10 */
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 20.0f, out);      /* Kp*error */
}

static void test_output_clamp(void) {
    PIDController p; base_pid(&p);
    p.Kp = 2.0f; p.limMax = 5.0f;
    float out = PIDController_Update(&p, 10.0f, 0.0f); /* raw 20 -> clamp 5 */
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 5.0f, out);
}

static void test_integrator_antiwindup(void) {
    PIDController p; base_pid(&p);
    p.Ki = 2.0f; p.T = 1.0f; p.limMaxInt = 3.0f;
    /* integrator = 0 + 0.5*Ki*T*(error+prevError) = 0.5*2*1*(10+0)=10 -> clamp 3 */
    float out = PIDController_Update(&p, 10.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 3.0f, out);
}

static void test_derivative_on_measurement_sign(void) {
    PIDController p; base_pid(&p);
    p.Kd = 1.0f;
    PIDController_Update(&p, 0.0f, 0.0f);             /* prime prevMeasurement=0 */
    float out = PIDController_Update(&p, 0.0f, 5.0f); /* measurement rose 0->5 */
    TEST_ASSERT_TRUE(out < 0.0f);                     /* derivative opposes rise */
}

static void test_p_only_converges_to_setpoint(void) {
    /* P controller on a pure-integrator plant (meas += T*out) converges to sp */
    PIDController p; base_pid(&p);
    p.Kp = 0.5f; p.T = 0.1f;
    float meas = 0.0f, sp = 100.0f;
    for (int i = 0; i < 500; i++) {
        float out = PIDController_Update(&p, sp, meas);
        meas += p.T * out;
    }
    TEST_ASSERT_FLOAT_WITHIN(0.5f, sp, meas);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_proportional_only);
    RUN_TEST(test_output_clamp);
    RUN_TEST(test_integrator_antiwindup);
    RUN_TEST(test_derivative_on_measurement_sign);
    RUN_TEST(test_p_only_converges_to_setpoint);
    return UNITY_END();
}
