/*
 * str_utils.h
 *
 *  Created on: May 23, 2023
 *      Author: mannk
 */

#ifndef SRC_STR_UTILS_H_
#define SRC_STR_UTILS_H_

#include <stdint.h>

uint32_t str_len(const char* str);
uint8_t str_compare_n(const char* str1, const char* str2, uint32_t n);
char* str_cpy_n(char* str_dest, char* str_src, uint32_t n);
void str_reverse(char *str);

#endif /* SRC_STR_UTILS_H_ */
