/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "wrld/input/input.h"
#include <assert.h>
#include <string.h>

void input_set_mouse_pos_n_scroll(InputState* in, int x, int y, float wheel) {
    in->mouse.dx = in->mouse.x - x;
    in->mouse.dy = in->mouse.y - y;
    in->mouse.x = x;
    in->mouse.y = y;
    in->mouse.wheel = wheel;
}

void input_press_mouse_btn(InputState* in, BUTTON btn) {
    if (btn >= MOUSE_BUTTON_COUNT) return;
    const uint8_t BTN = 1 << btn;

    in->mouse.btns |= BTN;
    in->mouse.btns_pressed |= BTN;
}

bool input_button_was_pressed(InputState* in, BUTTON btn) {
    assert(btn < MOUSE_BUTTON_COUNT);
    return in->mouse.btns_pressed & (1 << btn);
}

void input_release_mouse_btn(InputState* in, BUTTON btn) {
    if (btn >= MOUSE_BUTTON_COUNT) return;
    const uint8_t BTN = 1 << btn;
    in->mouse.btns &= ~BTN;
    in->mouse.btns_pressed &= ~BTN;
    in->mouse.btns_released |= BTN;
}

bool input_button_was_released(InputState* in, BUTTON btn) {
    assert(btn < MOUSE_BUTTON_COUNT);
    return in->mouse.btns_released & (1 << btn);
}

void input_press_key(InputState* in, KeyCode key) {
    if (key >= KEY_COUNT) return;
    const uint8_t KEY = 1 << (key % 8);
    const uint8_t idx = key / 8;

    in->keys[idx] |= KEY;
    in->keys_pressed[idx] |= KEY;
}


bool input_key_was_pressed(InputState* in, KeyCode key) {
    if (key >= KEY_COUNT) return false;
    const uint8_t KEY = 1 << (key % 8);
    const uint8_t idx = key / 8;
    return in->keys_pressed[idx]& (1 << KEY);
}

void input_release_key(InputState* in, KeyCode key) {
    if (key >= KEY_COUNT) return;
    const uint8_t KEY = 1 << (key % 8);
    const uint8_t idx = key / 8;

    in->keys[idx] &= ~KEY;
    in->keys_pressed[idx] &= ~KEY;
    in->keys_released[idx] |= KEY;
}

bool input_key_was_released(InputState* in, KeyCode key) {
    if (key >= KEY_COUNT) return false;
    const uint8_t KEY = 1 << (key % 8);
    const uint8_t idx = key / 8;
    return in->keys_released[idx]& (1 << KEY);
}

void input_reset(InputState* in) {
    in->mouse.btns_released = 0;
    in->mouse.btns_pressed = 0;

    memset(in->keys_pressed, 0, sizeof(in->keys_pressed));
    memset(in->keys_released, 0, sizeof(in->keys_released));
}

