#pragma once

#include <stdint.h>

/**
 * @brief Injects raw arbitrary 802.11 frame
 * @param *frame_buffer Buffer that contains the frame to be injected
 * @param size Size of the frame buffer
 */
void inject_frame(uint8_t *frame_buffer, int size);