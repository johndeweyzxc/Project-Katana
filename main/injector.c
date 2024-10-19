/**
 * @file injector.c
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief This component is use to bypass the ESP32's ability to block
 * transmission of raw 802.11 frame, this is use by deauther function in
 * exploits.c
 */

#include "esp_err.h"
#include "esp_wifi.h"

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  return 0;
}

void inject_frame(uint8_t *frame_buffer, int size) {
  ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false));
}