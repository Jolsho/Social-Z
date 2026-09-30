
/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "wrld/input/keys.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t     x;
    int32_t     y;
    int32_t     dx;
    int32_t     dy;
    float       wheel;

    uint8_t     btns;          // currently held
    uint8_t     btns_pressed;  // pressed this frame
    uint8_t     btns_released; // released this frame
} Mouse;

typedef struct {
    Mouse       mouse;
    uint64_t    keys[2];          // currently held
    uint64_t    keys_pressed[2];  // pressed this frame
    uint64_t    keys_released[2]; // released this frame
} InputState;

bool input_button_was_pressed(InputState* in, BUTTON btn);
bool input_key_was_pressed(InputState* in, KeyCode key);
bool input_button_was_released(InputState* in, BUTTON btn);
bool input_key_was_released(InputState* in, KeyCode key);
