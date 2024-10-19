#pragma once
#include "esp_netif.h"

/**
 * @brief Set a network interface for DHCP when Wi-Fi initializes
 * @param new_netif Network interface to set
 */
void dhcp_server_set_netif(esp_netif_t* new_netif);

/**
 * @brief Stop DHCP server
 */
void dhcp_server_stop();

/**
 * @brief Starts DHCP server
 */
void dhcp_server_start();