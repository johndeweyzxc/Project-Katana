/**
 * @file frame_parser.h
 * @author johndeweyzxc (johndewey02003@gmail.com)
 * @brief Declaration of methods for parsing frame data received from monitor mode of wifi in wifi_ctl
 */

#pragma once

#include <stdint.h>

/**
 * @brief Argument for frame_parser_set_target_parameter(), this is use to determine if armament is PMKID or MIC
 */
enum parse_type_list { PARSE_MIC, PARSE_PMKID, NULL_PARSE_TYPE };

/**
 * @brief Used by frame_parser_register_data_frame_handler() as an event id for determining frame type. Though only DATA_FRAME is the only one being used
 */
enum frame_event_id { DATA_FRAME, MGMT_FRAME, CTRL_FRAME, MISC_FRAME };

/**
 * @brief Unregister event handle data_frame_parser()
 */
void frame_parser_unregister_data_frame_handler();

/**
 * @brief Register event handler data_frame_parser()
 */
void frame_parser_register_data_frame_handler();

/**
 * @brief Clears the value of target MAC address and parse type
 */
void frame_parser_clear_target_parameter();

/**
 * @brief Sets the target MAC address and the parse type
 * @param *target_bssid MAC address of target (6 bytes) access point
 * @param selected_parse_type Key type to parse, either PARSE_PMKID or PARSE_MIC
 */
void frame_parser_set_target_parameter(uint8_t *target_bssid,
                                       uint8_t selected_parse_type);