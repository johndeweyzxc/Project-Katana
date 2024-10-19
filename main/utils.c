/**
 * @file utils.c
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Implements methods for converting a hexadecimal string to unsigned 8
 * bit integer
 */

#include "utils.h"

#include <string.h>

#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint8_t random_8bit() { return esp_random() & 0xFF; }

/**
 * @brief Concatenate two character buffer
 * @param *s1 Buffer of first buffer
 * @param *s2 Buffer of second buffer
 */
char *str_append(char *s1, char *s2) {
  int s1_length = strlen(s1);
  int s2_length = strlen(s2);
  int size = s1_length + s2_length;
  char *s = calloc(size, sizeof(char));

  for (int i = 0; i < s1_length; i++) {
    s[i] = s1[i];
  }
  for (int i = 0; i < s2_length; i++) {
    s[s1_length + i] = s2[i];
  }
  return s;
}

/**
 * @brief Converts hexadecimal string into unsigned 8 bit integer
 * @param *hex_string Buffer of hexadecimal characters
 * @param *result Buffer to store converted unsigned 8 bit integer from
 * hexadecimal characters
 */
void hex_str_to_bytes(const char *hex_string, uint8_t *result) {
  int hex_length = strlen(hex_string);

  for (int i = 0; i < hex_length; i += 2) {
    vTaskDelay(25 / portTICK_PERIOD_MS);
    sscanf(hex_string + i, "%2hhx", &result[i / 2]);
  }
}

/**
 * @brief Convert unsigned 8 bit integer into character then output the said
 * character
 * @param *buff Buffer of unsigned 8 bit integer
 * @param buff_len Length of *buff
 */
void print_uint8t_into_char(uint8_t *buff, uint8_t buff_len) {
  int len = buff_len - 12;
  for (int i = 0; i < len; i++) {
    if (buff[i] == 0) {
      break;
    }
    printf("%c", buff[i]);
  }
  printf("\n");
}

uint8_t util_convert_to_uint8_t(char s1, char s2) {
  char *s;
  uint8_t uint8_bit;
  if (s1 == '\0' || s2 == '\0') return 0;
  char s1a[] = {s1, '\0'};
  char s2a[] = {s2, '\0'};
  s = str_append(s1a, s2a);
  uint8_bit = strtol(s, NULL, 16);
  free(s);
  return uint8_bit;
}

void util_print_uint8t_into_hex(uint8_t *buff, int buff_len) {
  for (int i = 0; i < buff_len; i++) {
    if (buff[i] == 0) {
      break;
    }
    printf("%02X", buff[i]);
  }
  printf(",");
}