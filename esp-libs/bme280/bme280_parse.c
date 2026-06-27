#include "bme280_parse.h"

/* Read a little-endian unsigned 16-bit value from two consecutive bytes. */
static uint16_t u16le(const uint8_t *p)
{
    return (uint16_t) ((uint16_t) p[1] << 8 | p[0]);
}

/* Read a little-endian signed 16-bit value from two consecutive bytes. */
static int16_t s16le(const uint8_t *p)
{
    return (int16_t) u16le(p);
}

/* Sign-extend a 12-bit value held in the low 12 bits of v. */
static int16_t sx12(uint16_t v)
{
    v &= 0x0FFF;
    if (v & 0x0800) {
        v |= 0xF000;
    }
    return (int16_t) v;
}

bool bme280_parse_calib(const uint8_t calib26[26], const uint8_t calibH[7],
                        bme280_calib *out)
{
    if (calib26 == NULL || calibH == NULL || out == NULL) {
        return false;
    }

    /* Temperature / pressure block, base register 0x88. */
    out->T1 = u16le(&calib26[0]);
    out->T2 = s16le(&calib26[2]);
    out->T3 = s16le(&calib26[4]);
    out->P1 = u16le(&calib26[6]);
    out->P2 = s16le(&calib26[8]);
    out->P3 = s16le(&calib26[10]);
    out->P4 = s16le(&calib26[12]);
    out->P5 = s16le(&calib26[14]);
    out->P6 = s16le(&calib26[16]);
    out->P7 = s16le(&calib26[18]);
    out->P8 = s16le(&calib26[20]);
    out->P9 = s16le(&calib26[22]);
    /* calib26[24] = 0xA0 reserved; calib26[25] = 0xA1 = H1. */
    out->H1 = calib26[25];

    /* Humidity block, base register 0xE1. H4/H5 are packed 12-bit signed:
     *   H4 = (0xE4 << 4) | (0xE5 & 0x0F)
     *   H5 = (0xE6 << 4) | (0xE5 >> 4) */
    out->H2 = s16le(&calibH[0]);
    out->H3 = calibH[2];
    out->H4 = sx12((uint16_t) ((uint16_t) calibH[3] << 4 | (calibH[4] & 0x0F)));
    out->H5 = sx12((uint16_t) ((uint16_t) calibH[5] << 4 | (calibH[4] >> 4)));
    out->H6 = (int8_t) calibH[6];

    return true;
}

bool bme280_compensate(const bme280_calib *cal, int32_t adc_T, int32_t adc_P,
                       int32_t adc_H, bme280_reading *out)
{
    if (cal == NULL || out == NULL) {
        return false;
    }

    /* Temperature (Bosch BME280_compensate_T_int32), result in 0.01 C. */
    int32_t var1, var2, t_fine, T;
    var1 = ((((adc_T >> 3) - ((int32_t) cal->T1 << 1))) * ((int32_t) cal->T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t) cal->T1)) *
              ((adc_T >> 4) - ((int32_t) cal->T1))) >> 12) *
            ((int32_t) cal->T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    out->temp_mc = T * 10;   /* 0.01 C -> milli-C */

    /* Pressure (Bosch BME280_compensate_P_int64), result Q24.8 Pa. */
    int64_t p1, p2, p;
    p1 = (int64_t) t_fine - 128000;
    p2 = p1 * p1 * (int64_t) cal->P6;
    /* multiply, NOT `<< n`: left-shifting a possibly-negative value is UB
       (C11 6.5.7p4); x * 2^n is the identical value and well-defined for
       negatives (p1 and the signed P-coeffs can be negative). */
    p2 = p2 + ((p1 * (int64_t) cal->P5) * 131072);                /* was << 17 */
    p2 = p2 + ((int64_t) cal->P4 * ((int64_t) 1 << 35));          /* was << 35 */
    p1 = ((p1 * p1 * (int64_t) cal->P3) >> 8) + ((p1 * (int64_t) cal->P2) * 4096); /* was << 12 */
    p1 = (((((int64_t) 1) << 47) + p1) * ((int64_t) cal->P1)) >> 33;
    if (p1 == 0) {
        out->press_pa = 0;   /* avoid division by zero */
    } else {
        p = 1048576 - adc_P;
        p = (((p * ((int64_t) 1 << 31)) - p2) * 3125) / p1;   /* p<<31 as multiply */
        p1 = (((int64_t) cal->P9) * (p >> 13) * (p >> 13)) >> 25;
        p2 = (((int64_t) cal->P8) * p) >> 19;
        p = ((p + p1 + p2) >> 8) + ((int64_t) cal->P7 * 16);   /* was << 4 (P7 may be negative) */
        out->press_pa = (uint32_t) ((uint64_t) p >> 8);   /* Q24.8 -> Pa */
    }

    /* Humidity (Bosch BME280_compensate_H_int32), result Q22.10 %RH. */
    int32_t h;
    h = t_fine - 76800;
    h = ((((adc_H << 14) - ((int32_t) cal->H4 * 1048576) - (((int32_t) cal->H5) * h)) +
          16384) >> 15) *
        (((((((h * ((int32_t) cal->H6)) >> 10) *
             (((h * ((int32_t) cal->H3)) >> 11) + 32768)) >> 10) + 2097152) *
              ((int32_t) cal->H2) + 8192) >> 14);
    h = h - (((((h >> 15) * (h >> 15)) >> 7) * ((int32_t) cal->H1)) >> 4);
    h = h < 0 ? 0 : h;
    h = h > 419430400 ? 419430400 : h;
    uint32_t humi_q22_10 = (uint32_t) h >> 12;
    out->humi_mrh = humi_q22_10 * 1000u / 1024u;   /* Q22.10 %RH -> milli-%RH */

    return true;
}
