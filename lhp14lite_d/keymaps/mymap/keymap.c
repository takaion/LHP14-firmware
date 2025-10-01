// Copyright 2025 Neo Trinity
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "joystick.h"
#include "analog.h"

// Joystick configurations
// Debug mode: show the joystick angles to OLED (comment out or undef this to disable)
#define JS_DEBUG_ENABLED
// Enable/disable stick (Buttons are always enabled)
static bool is_js_enabled = true;
// Deadzone: stick tilting values lower than this value are ignored
#define JS_DEADZONE 64
// Pins
#define JS_PIN_X F5
#define JS_PIN_Y F4
// ADC Measured value
#define JS_X_MIN 172
#define JS_X_MED 444
#define JS_X_MAX 784

#define JS_Y_MIN 244
#define JS_Y_MED 532
#define JS_Y_MAX 822

struct JOYSTICK_ANGLES { int16_t x; int16_t y; };

// Button repeating
#define JS_RAPID_BUTTON 1
#define JS_RAPID_INTERVAL 60
static bool is_js_rapid_enabled = false;
static bool is_js_rapid_pressing = false;
static uint16_t js_rapid_timer = 0;

// Layers
// Max 32 layers available
#define MAIN 0
#define NUMPADS 1
#define FUNCTIONS 2
#define TEST 3

void render_logo(void) {
    for (uint8_t i = 0; i < LOGO_LINES; i++) {
        oled_set_cursor(0, i);
        oled_write_P(lhp_logo[i], false);
    }
};

enum custom_keycodes {
    RGBRST = SAFE_RANGE,
    DOUBLE_ZERO,
    JS_TOGGLE,
    JS_RAPID,
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case RGBRST:
            #ifdef RGBLIGHT_ENABLE
            if (record->event.pressed) {
                eeconfig_update_rgblight_default();
                rgblight_enable();
            }
            #endif
            break;
        case DOUBLE_ZERO:
            if (record->event.pressed) {
                SEND_STRING("00");
            }
            break;
        case JS_TOGGLE:
            if (record->event.pressed) {
                is_js_enabled = !is_js_enabled;
            }
            break;
        case JS_RAPID:
            if (record->event.pressed) {
                is_js_rapid_enabled = !is_js_rapid_enabled;
                js_rapid_timer = timer_read();
                if (!is_js_rapid_enabled) unregister_joystick_button(JS_RAPID_BUTTON);
            }
    }
    return true;
};

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

void read_joystick_angles(struct JOYSTICK_ANGLES *result) {
    if (!is_js_enabled) {
        result->x = 0;
        result->y = 0;
        return;
    }
    struct JOYSTICK_ANGLES raw = { analogReadPin(JS_PIN_X), analogReadPin(JS_PIN_Y) };
    bool is_dz = is_in_deadzone(raw.x - JS_X_MED, raw.y - JS_Y_MED, JS_DEADZONE);
    if (is_dz) {
        result->x = 0;
        result->y = 0;
        return;
    }
    result->x = joystick_angle(raw.x, JS_X_MAX, JS_X_MED, JS_X_MIN);
    result->y = joystick_angle(raw.y, JS_Y_MIN, JS_Y_MED, JS_Y_MAX);
}

void render_joystick_angle(int16_t val) {
    // フラッシュメモリ節約のためsprintfの代わりを自前で実装する
    char c;
    bool is_nonzero_printed = false, is_negative = val < 0;
    if (is_negative) val *= -1;
    c = (val / 100) + 0x30;
    if (c != '0') {
        oled_write_char(is_negative && !is_nonzero_printed ? '-' : ' ', false);
        oled_write_char(c, false);
        is_nonzero_printed = true;
    } else {
        oled_write_char(' ', false);
    }
    val = val % 100;
    c = (val / 10) + 0x30;
    if (c != '0') {
        if (!is_nonzero_printed) oled_write_char(is_negative ? '-' : ' ', false);
        oled_write_char(c, false);
        is_nonzero_printed = true;
    } else {
        oled_write_char(is_nonzero_printed ? c : ' ', false);
    }
    if (!is_nonzero_printed) oled_write_char(is_negative ? '-' : ' ', false);
    oled_write_char((val % 10) + 0x30, false);
}

void render_joystick_angles(struct JOYSTICK_ANGLES *angles) {
    oled_set_cursor(0, 3);
    oled_write_P(PSTR("X:"), false);
    render_joystick_angle(angles->x);
    oled_write_P(PSTR(" Y:"), false);
    render_joystick_angle(angles->y);
}

void report_joystick(void) {
    // The resolution of ADCs are 10bit: 0 - 1023
    // A virtual joystick has a range of -128 - 127 (int8_t)
    struct JOYSTICK_ANGLES angles;
    read_joystick_angles(&angles);
    joystick_set_axis(0, angles.x); // X軸
    joystick_set_axis(1, angles.y); // Y軸
    joystick_flush();
    #ifdef JS_DEBUG_ENABLED
    render_joystick_angles(&angles);
    #endif
}

void run_js_rapid(void) {
    if (is_js_rapid_enabled) {
        // 前回の連打イベントから指定時間以上経過したかチェック
        if (timer_elapsed(js_rapid_timer) >= JS_RAPID_INTERVAL) {
            // ジョイスティックボタンの押下と即時解放をシミュレート
            if (is_js_rapid_pressing) {
                unregister_joystick_button(JS_RAPID_BUTTON);
            } else {
                register_joystick_button(JS_RAPID_BUTTON);
            }
            is_js_rapid_pressing = !is_js_rapid_pressing;
            // タイマーをリセット
            js_rapid_timer = timer_read();
        }
    }
}

void matrix_scan_user(void) {
    run_js_rapid();
    report_joystick();
}

joystick_config_t joystick_axes[JOYSTICK_AXIS_COUNT] = {
    JOYSTICK_AXIS_VIRTUAL,
    JOYSTICK_AXIS_VIRTUAL,
};

void render_lock_state(void) {
    led_t led_state = host_keyboard_led_state();
    oled_set_cursor(13, 0);
    oled_write_P(led_state.num_lock ? PSTR("NL ") : PSTR("   "), false);
    oled_write_P(led_state.caps_lock ? PSTR("CL ") : PSTR("   "), false);
    oled_write_P(led_state.scroll_lock ? PSTR("SL") : PSTR("  "), false);
}

void render_js_state(void) {
    oled_set_cursor(13, 1);
    oled_write_P(PSTR("JS:"), false);
    oled_write_P(is_js_enabled ? PSTR("E") : PSTR("D"), false);
    oled_write_P(is_js_rapid_enabled ? PSTR("R") : PSTR("-"), false);
}

void render_layer_name(const char* name) {
    oled_set_cursor(0, 2);
    oled_write_P(PSTR("Layer: "), false);
    oled_write_ln_P(name, false);
}

void render_layer(void) {
    // Maximum number of the length of the layer name is 14
    switch (get_highest_layer(layer_state)) {
        case MAIN:
            render_layer_name(PSTR("MAIN"));
            #ifdef RGBLIGHT_ENABLE
            rgblight_sethsv(0, 255, 90);
            #endif
            break;
        case NUMPADS:
            render_layer_name(PSTR("NUMPADS"));
            break;
        case FUNCTIONS:
            render_layer_name(PSTR("FUNCTIONS"));
            break;

        case TEST:
            render_layer_name(PSTR("TEST"));
            break;
            
        default:
            // Or use the write_ln shortcut over adding '\n' to the end of your string
            render_layer_name(PSTR("Undefined"));
    }
};

bool oled_task_user(void) {
    render_logo();
    render_layer();
    render_lock_state();
    render_js_state();
    return false;
};

void suspend_power_down_kb(void) {
    oled_off();
};

void suspend_wakeup_init_kb(void) {
    oled_on();
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* MAIN
     * ,----------------------------------.   
     * |      |      |      |      |      |   
     * |------+------+------+------+------|   
     * |      |      |      |      |      |   
     * |------+------+------+------+------|   
     * |      |      |      |      |      |   
     * |------+------+------+------+------+------+------.  
     * |      |      |      |      |      |JsPush|TEST  |  
     * `------------------------------------------------'  
     */
    [MAIN] = LAYOUT( \
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, JS_TOGGLE, \
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, JS_RAPID, \
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, JS_0, TO(NUMPADS) \
    ),

    [NUMPADS] = LAYOUT( \
        KC_NUM,  KC_P7, KC_P8,       KC_P9,   LSFT(KC_MINS), \
        KC_PSLS, KC_P4, KC_P5,       KC_P6,   KC_PMNS, \
        KC_PAST, KC_P1, KC_P2,       KC_P3,   KC_PPLS, \
        KC_BSPC, KC_P0, DOUBLE_ZERO, KC_PDOT, KC_PENT, JS_0, TO(FUNCTIONS)
    ),

    [FUNCTIONS] = LAYOUT( \
        JS_TOGGLE, KC_F22, KC_F23, KC_F24, KC_MPLY, \
        KC_MSTP,   KC_F19, KC_F20, KC_F21, KC_VOLD, \
        KC_MUTE,   KC_F16, KC_F17, KC_F18, KC_VOLU, \
        KC_MPRV,   KC_F13, KC_F14, KC_F15, KC_MNXT, JS_0, TO(TEST) \
    ),

    /* TEST
     * ,----------------------------------.   
     * |RGBTOG|RGBHUI|RGBHUD|RGBSAI|RGBSAD|   
     * |------+------+------+------+------|   
     * |RGBMOD|RGBRST|RGBVAI|RGBVAD|      |   
     * |------+------+------+------+------|   
     * |      |      |      |      |      |   
     * |------+------+------+------+------+------+------.  
     * |      |      |      |      |      |JsPush|MAIN3  |  
     * `------------------------------------------------'  
     */
    [TEST] = LAYOUT( \
        UG_TOGG, UG_HUEU, UG_HUED, UG_SATU, UG_SATD, \
        UG_NEXT, RGBRST,  UG_VALU, UG_VALD, XXXXXXX, \
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, JS_0, TO(MAIN) \
    ),
};
