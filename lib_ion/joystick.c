#include QMK_KEYBOARD_H
#include "analog.h"
#include "joystick.h"
#include "lib_ion/joystick.h"

bool is_in_deadzone(int16_t x, int16_t y, uint16_t dz) {
    // x, y はそれぞれ -512 - 511 程度なので2乗しても高々2^19程度に収まる
    uint32_t squared_length = (uint32_t)x * x + (uint32_t)y * y;
    uint32_t squared_deadzone= (uint32_t)dz * dz;
    return squared_length < squared_deadzone;
} 

int16_t joystick_angle(int16_t raw, int16_t min, int16_t mid, int16_t max) {
    int32_t val = (raw - mid) * JOYSTICK_MAX_VALUE / (mid - min);
    // 負の値であるはずが正の値になっている場合はおかしいので高い方でやり直し
    if (val > 0) val = (raw - mid) * JOYSTICK_MAX_VALUE / (max - mid);
    // 最小値・最大値でクリップする
    if (val < -JOYSTICK_MAX_VALUE) val = -JOYSTICK_MAX_VALUE;
    if (val > JOYSTICK_MAX_VALUE) val = JOYSTICK_MAX_VALUE;
    return val; // min と max の大小関係が逆なら正負反転して返す
}

void read_joystick_angles(struct JOYSTICK_STATE *state) {
    if (!state->enabled) {
        state->x = 0;
        state->y = 0;
        return;
    }
    struct JOYSTICK_ANGLES raw = { analogReadPin(JS_PIN_X), analogReadPin(JS_PIN_Y) };
    bool is_dz = is_in_deadzone(raw.x - JS_X_MED, raw.y - JS_Y_MED, JS_DEADZONE);
    if (is_dz) {
        state->x = 0;
        state->y = 0;
        return;
    }
    state->x = joystick_angle(raw.x, JS_X_MAX, JS_X_MED, JS_X_MIN);
    state->y = joystick_angle(raw.y, JS_Y_MIN, JS_Y_MED, JS_Y_MAX);
}

void report_joystick(struct JOYSTICK_STATE *state, uint8_t x_axis, uint8_t y_axis) {
    // The resolution of ADCs are 10bit: 0 - 1023
    // A virtual joystick has a range of -128 - 127 (int8_t)
    read_joystick_angles(state);
    joystick_set_axis(x_axis, state->x); // X軸
    joystick_set_axis(y_axis, state->y); // Y軸
    joystick_flush();
}


void start_joystick_rapid(struct JOYSTICK_RAPID_STATE *state) {
    state->enabled = true;
    state->timer = timer_read();
}

void stop_joystick_rapid(struct JOYSTICK_RAPID_STATE *state) {
    state->enabled = false;
    unregister_joystick_button(state->button);
}

void toggle_joystick_rapid(struct JOYSTICK_RAPID_STATE *state) {
    if (state->enabled) stop_joystick_rapid(state);
    else start_joystick_rapid(state);
}

void run_joystick_rapid(struct JOYSTICK_RAPID_STATE *state) {
    if (!state->enabled || timer_elapsed(state->timer) < JS_RAPID_INTERVAL) return;
    if (state->pressing) {
        unregister_joystick_button(state->button);
    } else {
        register_joystick_button(state->button);
    }
    state->pressing = !state->pressing;
    state->timer = timer_read();
}