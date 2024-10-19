/**
 * @file utils.h
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Declaration of methods for converting a hexadecimal string to unsigned
 * 8 bit integer
 */

#pragma once
#include "esp_wifi_types.h"

/**
 * @brief Generate random 8 bit unsigned integer
 */
uint8_t random_8bit();

/**
 * @brief Utility function for converting one hexadecimal ('0A') string to 8 bit
 * unsigned integer
 * @param s1 First character from hexadecimal string ('0')
 * @param s2 Second character from hexadecimal string ('A')
 */
uint8_t util_convert_to_uint8_t(char s1, char s2);

/**
 * @brief Utility function for printing an array of 8 bit unsigned integer into
 * hexadecimal string
 * @param *buff Buffer of 8 bit unsigned integer
 * @param buff_len Length of buffer
 */
void util_print_uint8t_into_hex(uint8_t *buff, int buff_len);