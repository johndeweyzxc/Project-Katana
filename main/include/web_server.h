#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Command mode password verification indicator after calculating and comparing MIC or PMKID
 */
#define PSK_CORRECT 8
#define PSK_INCORRECT 9
#define PSK_NO_REPLY 10

/**
 * @brief Set PSK_CORRECT if the password entered is correct, PSK_INCORRECT if incorrect
 * @param status
 */
void web_psk_status(uint8_t status);

/**
 * @brief Stop web server
 */
void web_server_stop();

/**
 * @brief Start web server
 */
void web_server_start();
