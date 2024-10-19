/**
 * @file frame_output.h
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Declaration of methods for formatting and outputting PMKID and MIC from EAPOL data.
 */

#pragma once

#include "eapol_frame.h"

/**
 * @brief Puts the data in the output buffer char_buff and prints the data
 * @param *message_1 EAPOL data of first message from 4 way handshake
 */
void output_pmkid(eapol_frame_t *message_1);

/**
 * @brief Puts the bssid, transmitter address and access point nonce in the output buffer then prints the data
 * @param *message_1 EAPOL data of first message from 4 way handshake
 */
void output_anonce_from_message_1(eapol_frame_t *message_1);

/**
 * @brief Puts the bssid, transmitter address, station nonce, mic, and key data in the output buffer *char_buff then prints the data
 * @param *message_2 EAPOL data of the second message from 4 way handshake
 */
void output_mic_from_message_2(eapol_frame_t *message_2);