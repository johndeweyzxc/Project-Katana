/**
 * @file cmd_parser.c
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Implements functionality for parsing and interpreting command inputs
 * from serial then executing those commands
 */

#include "cmd_parser.h"

#include <stdio.h>
#include <string.h>

#include "deauther.h"
#include "dhcp_server.h"
#include "dns_server.h"
#include "frame_parser.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils.h"
#include "web_server.h"
#include "wifi_ctl.h"

static TaskHandle_t cmd_parser_task_handle = NULL;

void set_user_in_buff_to_zero(uint8_t *user_in_buff) {
  for (uint8_t i = 0; i < MAX_INPUT_SIZE; i++) {
    user_in_buff[i] = 0;
  }
}

void print_current_command(uint8_t *user_in_buff) {
  printf("{CMD_PARSER,CURRENT_CMD,");
  for (uint8_t i = 0; i < 44; i++) {
    printf("%c", user_in_buff[i]);
  }
  printf(",}\n");
}

void convert_input_to_uint8t(uint8_t *user_in_buff, cmd_arg_t *u_user_in_buff) {
  uint8_t mode = util_convert_to_uint8_t(user_in_buff[0], user_in_buff[1]);
  uint8_t key_type = util_convert_to_uint8_t(user_in_buff[2], user_in_buff[3]);
  uint8_t target_wifi_channel =
      util_convert_to_uint8_t(user_in_buff[4], user_in_buff[5]);
  uint8_t target_wifi_name_length =
      util_convert_to_uint8_t(user_in_buff[6], user_in_buff[7]);

  u_user_in_buff->mode = mode;
  u_user_in_buff->key_type = key_type;
  u_user_in_buff->target_wifi_channel = target_wifi_channel;
  u_user_in_buff->target_wifi_name_length = target_wifi_name_length;

  uint8_t *p_wifi_mac = user_in_buff;
  p_wifi_mac += 8;
  for (uint8_t i = 0; i < MAC_SIZE; i++) {
    uint8_t s1 = p_wifi_mac[i + i];
    uint8_t s2 = p_wifi_mac[i + i + 1];
    u_user_in_buff->target_wifi_mac[i] = util_convert_to_uint8_t(s1, s2);
  }

  uint8_t *p_wifi_name = user_in_buff;
  p_wifi_name += 20;
  for (uint8_t i = 0; i < target_wifi_name_length; i++) {
    uint8_t s1 = p_wifi_name[i + i];
    uint8_t s2 = p_wifi_name[i + i + 1];
    u_user_in_buff->target_wifi_name[i] = util_convert_to_uint8_t(s1, s2);
  }
}

void assemble_input(cmd_arg_t *u_user_in_buff, uint8_t *target_ap_bssid,
                    uint8_t *target_ap_name) {
  memcpy(target_ap_bssid, u_user_in_buff->target_wifi_mac, MAC_SIZE);
  memcpy(target_ap_name, u_user_in_buff->target_wifi_name,
         u_user_in_buff->target_wifi_name_length);
}

void output_assembled_input(cmd_arg_t *u_user_in_buff, uint8_t *target_ap_bssid,
                            uint8_t *target_ap_name) {
  printf("{CMD_PARSER,PARSED_CMD,");
  printf("%u,", u_user_in_buff->mode);
  printf("%u,", u_user_in_buff->target_wifi_channel);
  printf("%u,", u_user_in_buff->target_wifi_name_length);

  for (uint8_t i = 0; i < MAC_SIZE; i++) {
    printf("%02X", target_ap_bssid[i]);
  }
  printf(",");
  for (uint8_t i = 0; i < u_user_in_buff->target_wifi_name_length; i++) {
    printf("%02X", target_ap_name[i]);
  }
  printf(",}\n");
}

bool isModeNull(uint8_t *user_in_buff) {
  uint8_t mode = util_convert_to_uint8_t(user_in_buff[0], user_in_buff[1]);
  if (mode == NULL_MODE) {
    return true;
  }
  return false;
}

void cmd_parser() {
  uint8_t user_in_buff[MAX_INPUT_SIZE];
  set_user_in_buff_to_zero(user_in_buff);

  cmd_arg_t u_user_in_buff;

  while (1) {
    vTaskDelay(CMD_INPUT_DELAY / portTICK_PERIOD_MS);
    scanf("%82s", user_in_buff);

    // print_current_command(user_in_buff);

    if (isModeNull(user_in_buff)) {
      continue;
    }
    convert_input_to_uint8t(user_in_buff, &u_user_in_buff);

    // assemble_input(&u_user_in_buff, target_ap_bssid, target_ap_name);
    // output_assembled_input(&u_user_in_buff, target_ap_bssid, target_ap_name);

    switch (u_user_in_buff.mode) {
      case NULL_MODE:
        break;
      case DEAUTH_START_MODE:
        deauther_start_task(u_user_in_buff.target_wifi_channel,
                            u_user_in_buff.target_wifi_mac);

        set_user_in_buff_to_zero(user_in_buff);
        break;
      case DEAUTH_STOP_MODE:
        deauther_stop_task();

        set_user_in_buff_to_zero(user_in_buff);
        break;
      case AP_START_MODE:
        wifi_ctl_start_ap(u_user_in_buff.target_wifi_name,
                          u_user_in_buff.target_wifi_name_length);
        dhcp_server_start();
        web_server_start();
        dns_server_start_task();

        set_user_in_buff_to_zero(user_in_buff);
        break;
      case AP_STOP_MODE:
        dns_server_stop_task();
        web_server_stop();
        dhcp_server_stop();
        wifi_ctl_stop_ap();

        set_user_in_buff_to_zero(user_in_buff);
        break;
      case SNIFF_START_MODE:
        frame_parser_set_target_parameter(u_user_in_buff.target_wifi_mac,
                                          u_user_in_buff.key_type);
        frame_parser_register_data_frame_handler();
        wifi_ctl_sniffer_start(u_user_in_buff.target_wifi_channel);

        set_user_in_buff_to_zero(user_in_buff);
        break;
      case SNIFF_STOP_MODE:
        frame_parser_clear_target_parameter();
        frame_parser_unregister_data_frame_handler();
        wifi_ctl_sniffer_stop();

        set_user_in_buff_to_zero(user_in_buff);
        break;
      case SCAN_MODE:
        wifi_scan_aps();
        wifi_get_scanned_aps();

        set_user_in_buff_to_zero(user_in_buff);
        break;
      case PSK_CORRECT:
        web_psk_status(PSK_CORRECT);

        set_user_in_buff_to_zero(user_in_buff);
        break;
      case PSK_INCORRECT:
        web_psk_status(PSK_INCORRECT);

        set_user_in_buff_to_zero(user_in_buff);
        break;
      case ESP_RESTART:
        esp_restart();
        break;
      case CLIENT_TARGET:
        // In this case, the MAC address of client is being set, not the target
        // access point
        deauther_set_client_target(u_user_in_buff.target_wifi_mac);

        set_user_in_buff_to_zero(user_in_buff);
        break;
      default:
        set_user_in_buff_to_zero(user_in_buff);
        break;
    }
  }
}

void cmd_parser_create_task() {
  xTaskCreatePinnedToCore((TaskFunction_t)cmd_parser, CMD_PARSER_TASK_NAME,
                          CMD_PARSER_STACK_SIZE, NULL, CMD_PARSER_TASK_PRIORITY,
                          &cmd_parser_task_handle, CMD_PARSER_CORE_TO_USE);
  printf("{CMD_PARSER,INITIALIZED,}\n");
}