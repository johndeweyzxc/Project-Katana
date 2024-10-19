#pragma once

#include <stdint.h>

#include "esp_event.h"
#include "esp_wifi_types.h"

/**
 * @brief Total maximum number of AP to scan
 */
#define MAX_SCAN_AP 10

/**
 * @brief wifi_get_scan_aps() will return ap_list_from_scan_t which contains the
 * list of nearby access points
 */
typedef struct {
  uint16_t count;
  wifi_ap_record_t ap_record_list[MAX_SCAN_AP];
} ap_list_from_scan_t;

/**
 * @brief Scan for nearby access point
 */
void wifi_scan_aps();

/**
 * @brief Output the nearby access points after wifi_scan_aps()
 */
void wifi_get_scanned_aps();

/**
 * @brief Start access point
 * @param ssid_name Buffer for SSID
 * @param ssid_name_len Length of SSID
 */
void wifi_ctl_start_ap(uint8_t* ssid_name, uint8_t ssid_name_len);

/**
 * @brief Stop access point
 */
void wifi_ctl_stop_ap();

/**
 * @brief Start monitor mode to capture packets
 * @param channel The channel to monitor from 1 to 14
 */
void wifi_ctl_sniffer_start(uint8_t channel);

/**
 * @brief Stop monitor mode
 */
void wifi_ctl_sniffer_stop();

/**
 * @brief Initializes the Wi-Fi controller
 */
void wifi_ctl_init();