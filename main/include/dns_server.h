#pragma once
#include <stdint.h>

typedef struct {
  uint16_t transaction_id;
  uint16_t flags;
  uint16_t questions;
  uint16_t answer_rrs;
  uint16_t authority_rrs;
  uint16_t additional_rrs;
  uint8_t queries_and_answers[100];
} dns_query_t;

typedef struct __attribute__((packed)) {
  uint16_t name;
  uint16_t type;
  uint16_t dns_class;
  uint32_t ttl;
  uint16_t data_len;
  uint8_t address[4];
} dns_query_answer_t;

// Task specification for DNS server
#define TASK_DNS_SERVER_NAME "DNS_SERVER"
#define TASK_DNS_SERVER_STACK_SIZE 4096
#define TASK_DNS_SERVER_PRIORITY 2
#define TASK_DNS_SERVER_CORE_TO_USE 1

/**
 * @brief Stop DNS server
 */
void dns_server_stop_task();

/**
 * @brief Start a DNS server to listen for any DNS queries from clients
 * connected to access point
 */
void dns_server_start_task();