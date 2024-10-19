#pragma once

#include <stdint.h>

#define MAX_INPUT_SIZE 82
#define MAX_AP_CHAR 32
#define MAC_SIZE 6

// Command modes
#define NULL_MODE 0
#define DEAUTH_START_MODE 1
#define DEAUTH_STOP_MODE 2
#define AP_START_MODE 3
#define AP_STOP_MODE 4
#define SNIFF_START_MODE 5
#define SNIFF_STOP_MODE 6
#define SCAN_MODE 7

/**
 * @brief See web_server.h file
 */
// #define PSK_CORRECT 8
// #define PSK_INCORRECT 9
// #define PSK_NO_REPLY 10

// Command to restart ESP32
#define ESP_RESTART 11
#define CLIENT_TARGET 12

typedef struct {
  uint8_t mode;
  uint8_t key_type;
  uint8_t target_wifi_channel;
  uint8_t target_wifi_name_length;
  uint8_t target_wifi_mac[6];
  uint8_t target_wifi_name[32];
} cmd_arg_t;

// Task specification for command receiver and parser
#define CMD_INPUT_DELAY 1000
#define CMD_PARSER_TASK_NAME "CMD_PARSER_TASK"
#define CMD_PARSER_STACK_SIZE 2048
#define CMD_PARSER_TASK_PRIORITY 2
#define CMD_PARSER_CORE_TO_USE 1

/**
 * @brief Initializes the command receiver
 */
void cmd_parser_create_task();
