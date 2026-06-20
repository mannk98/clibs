/*
 * str_utils.c
 *
 *  Created on: May 23, 2023
 *      Author: mannk
 */

#include "str_utils.h"

uint32_t str_len(const char* str){
	uint32_t len = 0;
	while(*str != '\0'){
		len++;
		str++;
	}
	return len;
}

uint8_t str_compare_n(const char* str1, const char* str2, uint32_t n) {
	int i = 0;
	while (str1[i] && str2[i] && n) {
		if (str1[i] != str2[i]) {
			return str1[i] - str2[i];
		}
		i++;
		n--;
	}
	return str1[i] && str2[i];
}

char* str_cpy_n(char* str_dest, char* str_src, uint32_t n){
	while(n-- && *str_src != '\0' ){
		*str_dest = *str_src;
		str_dest++;
		str_src++;
	}
	return str_dest;
}

void str_reverse(char *str) {
    int i = 0, j = strlen(str) - 1;
    char temp;

    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}
