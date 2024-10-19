/**
 * @file wifi_ctl.c
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Implements functionality for controlling the Wi-Fi of ESP32
 */

#include "wifi_ctl.h"

#include <stdio.h>
#include <string.h>

#include "dhcp_server.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "frame_parser.h"
#include "utils.h"

ESP_EVENT_DECLARE_BASE(FRAME_RECEIVED_EVENT_BASE);
static ap_list_from_scan_t ap_list;

void wifi_scan_aps() {
  ap_list.count = MAX_SCAN_AP;

  wifi_scan_config_t scan_config = {.ssid = NULL,
                                    .bssid = NULL,
                                    .channel = 0,
                                    .scan_type = WIFI_SCAN_TYPE_ACTIVE};
  ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
  ESP_ERROR_CHECK(
      esp_wifi_scan_get_ap_records(&ap_list.count, ap_list.ap_record_list));
}

void wifi_get_scanned_aps() {
  wifi_ap_record_t* ap_records = ap_list.ap_record_list;
  uint8_t total_scanned_aps = ap_list.count;

  printf("{WIFI_CTL,FOUND_APS,%u,}\n", total_scanned_aps);
  for (uint8_t i = 0; i < total_scanned_aps; i++) {
    wifi_ap_record_t ap_record = ap_records[i];
    uint8_t* bssid = ap_record.bssid;

    vTaskDelay(25 / portTICK_PERIOD_MS);

    printf("{WIFI_CTL,SCAN,%02X:%02X:%02X:%02X:%02X:%02X,", bssid[0], bssid[1],
           bssid[2], bssid[3], bssid[4], bssid[5]);
    util_print_uint8t_into_hex(ap_record.ssid, 33);
    printf("%d,%u,}\n", ap_record.rssi, ap_record.primary);
  }
}

static void wifi_ctl_event_ap_handler(void* arg, esp_event_base_t event_base,
                                      int32_t event_id, void* event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t* event =
        (wifi_event_ap_staconnected_t*)event_data;
    uint8_t* b = event->mac;
    printf("{WIFI_CTL,ON_DEVICE_CONNECT,%02X:%02X:%02X:%02X:%02X:%02X,}\n",
           b[0], b[1], b[2], b[3], b[4], b[5]);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t* event =
        (wifi_event_ap_stadisconnected_t*)event_data;
    uint8_t* b = event->mac;
    printf("{WIFI_CTL,ON_DEVICE_DISCONNECT,%02X:%02X:%02X:%02X:%02X:%02X,}\n",
           b[0], b[1], b[2], b[3], b[4], b[5]);
  }
}

void wifi_ctl_start_ap(uint8_t* ssid_name, uint8_t ssid_name_len) {
  ESP_ERROR_CHECK(esp_wifi_stop());

  wifi_config_t wifi_config = {
      .ap = {.ssid = {0},
             .ssid_len = ssid_name_len,
             .channel = 1,
             .authmode = WIFI_AUTH_OPEN,
             .max_connection = 4},
  };
  memcpy(wifi_config.ap.ssid, ssid_name, ssid_name_len);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

// TODO: Crash Scan
void wifi_ctl_stop_ap() { ESP_ERROR_CHECK(esp_wifi_stop()); }

void wifi_ctl_on_frame(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* frame = (wifi_promiscuous_pkt_t*)buf;

  uint8_t event_id;
  switch (type) {
    case WIFI_PKT_DATA:
      event_id = DATA_FRAME;
      break;
    case WIFI_PKT_MGMT:
      event_id = MGMT_FRAME;
      break;
    case WIFI_PKT_CTRL:
      event_id = CTRL_FRAME;
      break;
    case WIFI_PKT_MISC:
      event_id = MISC_FRAME;
      printf("wifi_ctl.wifi_ctl_on_frame > Got MISC frame\n");
      break;
    default:
      printf("wifi_ctl.wifi_ctl_on_frame > Unknown frame\n");
      return;
  }

  if (event_id == MISC_FRAME) {
    return;
  }

  size_t data_size = frame->rx_ctrl.sig_len + sizeof(wifi_promiscuous_pkt_t);
  ESP_ERROR_CHECK(esp_event_post(FRAME_RECEIVED_EVENT_BASE, event_id, frame,
                                 data_size, portMAX_DELAY));
}

void wifi_ctl_sniffer_start(uint8_t channel) {
  esp_wifi_set_promiscuous(true);
  if (channel < 1 || channel > 14) {
    printf("wifi_ctl.wifi_sniffer_start: Invalid wifi channel: %u\n", channel);
    return;
  }
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  printf("wifi_ctl.wifi_sniffer_start: Wifi channel set to %u\n", channel);
  esp_wifi_set_promiscuous_rx_cb(&wifi_ctl_on_frame);
  wifi_promiscuous_filter_t filter = {.filter_mask = 0};
  filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
  esp_wifi_set_promiscuous_filter(&filter);
  printf("wifi_ctl.wifi_sniffer_start: Promiscuous mode started\n");
}

void wifi_ctl_sniffer_stop() {
  esp_wifi_set_promiscuous(false);
  printf("wifi_ctl.wifi_sniffer_stop: Promiscuous mode stopped\n");
}

void wifi_ctl_init() {
  ESP_ERROR_CHECK(esp_netif_init());
  dhcp_server_set_netif(esp_netif_create_default_wifi_ap());

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ctl_event_ap_handler, NULL, NULL));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

  // Set MAC address of ESP32 to random
  uint8_t new_mac_ap[6] = {0xa8,          random_8bit(), random_8bit(),
                           random_8bit(), random_8bit(), random_8bit()};
  ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, new_mac_ap));

  ESP_ERROR_CHECK(esp_wifi_start());
  printf("{WIFI_CTL,INITIALIZED,}\n");
}