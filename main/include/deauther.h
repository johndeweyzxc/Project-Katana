#pragma once
#include <stdint.h>

// Task specification for deauthentication
#define TASK_DEAUTHER_NAME "DEAUTH"
#define TASK_DEAUTHER_STACK_SIZE 2048
#define TASK_DEAUTHER_PRIORITY 2
#define TASK_DEAUTHER_CORE_TO_USE 1

/**
 * @brief Sets the client target of target access point
 * @param client_target_mac MAC address of client target
 */
void deauther_set_client_target(uint8_t client_target_mac[6]);

/**
 * @brief Stop deauthentication
 */
void deauther_stop_task();

/**
 * @brief Start deauthentication
 * @param channel Channel of target access point which ranges between 1 and 14
 * @param target_ap_bssid MAC address of target access point
 */
void deauther_start_task(uint8_t channel, uint8_t target_ap_bssid[6]);