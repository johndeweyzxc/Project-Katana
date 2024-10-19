/**
 * @file dns_server.c
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Implements functionality for running a DNS server
 */

#include "dns_server.h"

#include <arpa/inet.h>
#include <lwip/err.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TaskHandle_t dns_server_task_handle = NULL;
static const uint8_t custom_ip[] = {172, 217, 28, 1};

void dns_server_run(void *pvParameters) {
  int server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (server_sock < 0) {
    printf("dns_server.dns_server_run: Failed to create socket.\n");
    vTaskDelete(NULL);
    return;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  // Set DNS port to 53
  server_addr.sin_port = htons(53);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    printf("dns_server.dns_server_run: Failed to bind socket.\n");
    close(server_sock);
    vTaskDelete(NULL);
    return;
  }

  // Change "EXPLOITS" to "DNS_SERVER"
  printf("{EXPLOITS,DNS_HIJACK_STARTED,}\n");

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    uint8_t buffer[2048];

    ssize_t recv_len =
        recvfrom(server_sock, buffer, sizeof(buffer), 0,
                 (struct sockaddr *)&client_addr, &client_addr_len);
    if (recv_len < 0) {
      printf("dns_server.dns_server_run: Failed to receive data.\n");
      continue;
    }

    dns_query_t *dns_query = (dns_query_t *)buffer;
    uint8_t queries_buffer[recv_len - 12];
    memcpy(queries_buffer, dns_query->queries_and_answers, recv_len - 12);

    /* Create DNS query response where the response is 172.217.28.1 */
    dns_query_answer_t answer = {
        .name = htons(0xc00c),
        .type = htons(0x0001),
        .dns_class = htons(0x0001),
        .ttl = htonl(0x0000003c),
        .data_len = htons(0x0004),
        .address = {custom_ip[0], custom_ip[1], custom_ip[2], custom_ip[3]},
    };

    uint8_t answer_buffer[sizeof(dns_query_answer_t)];
    memcpy(answer_buffer, &answer, sizeof(dns_query_answer_t));

    dns_query_t final_answer = {
        .transaction_id = dns_query->transaction_id,
        .flags = htons(0x8100),
        .questions = dns_query->questions,
        .answer_rrs = htons(0x0001),
        .authority_rrs = 0x0000,
        .additional_rrs = 0x0000,
        .queries_and_answers = {0},
    };

    /* Craft the response to be sent to the client */
    size_t size_of_queries = sizeof(queries_buffer);
    /* Copies the query in the buffer  */
    memcpy(final_answer.queries_and_answers, &queries_buffer, size_of_queries);
    uint8_t *ptr_queries = final_answer.queries_and_answers + size_of_queries;
    /* Then copy the response for that query in the buffer */
    memcpy(ptr_queries, &answer_buffer, sizeof(dns_query_answer_t));

    uint8_t payload[recv_len + sizeof(dns_query_answer_t)];
    memcpy(payload, &final_answer, recv_len + sizeof(dns_query_answer_t));

    struct sockaddr_in response_addr;
    response_addr.sin_family = AF_INET;
    response_addr.sin_port = client_addr.sin_port;
    response_addr.sin_addr.s_addr = client_addr.sin_addr.s_addr;

    /* Send response to client */
    sendto(server_sock, payload, sizeof(payload), 0,
           (struct sockaddr *)&response_addr, sizeof(response_addr));
  }
}

void dns_server_stop_task() {
  if (dns_server_task_handle != NULL) {
    vTaskDelete(dns_server_task_handle);
    dns_server_task_handle = NULL;
  }
}

void dns_server_start_task() {
  xTaskCreatePinnedToCore((TaskFunction_t)dns_server_run, TASK_DNS_SERVER_NAME,
                          TASK_DNS_SERVER_STACK_SIZE, NULL,
                          TASK_DNS_SERVER_PRIORITY, &dns_server_task_handle,
                          TASK_DNS_SERVER_CORE_TO_USE);
}