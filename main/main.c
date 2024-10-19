/**
 * @file main.c
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Entrypoint of the application
 */

#include <stdio.h>

#include "cmd_parser.h"
#include "esp_err.h"
#include "esp_event.h"
#include "web_server.h"
#include "wifi_ctl.h"

void app_main(void) {
  printf("{ESP_STARTED,}\n");
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  cmd_parser_create_task();
  wifi_ctl_init();
}