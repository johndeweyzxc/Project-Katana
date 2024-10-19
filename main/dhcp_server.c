/**
 * @file deauther.c
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Implements functionality for running DHCP server
 */

#include "esp_err.h"
#include "esp_netif.h"
#include "lwip/ip_addr.h"

static esp_netif_t* netif;
static bool is_netif_stopped = false;

void dhcp_server_set_netif(esp_netif_t* new_netif) { netif = new_netif; }

void dhcp_server_start() {
  // TODO (In progress): Crash Access Point
  if (!is_netif_stopped) {
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));
  }
  esp_netif_ip_info_t ip_info;
  IP4_ADDR(&ip_info.ip, 172, 217, 28, 1);
  IP4_ADDR(&ip_info.gw, 172, 217, 28, 1);
  IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
  ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));
  esp_netif_dhcps_start(netif);
}

void dhcp_server_stop() { ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif)); }