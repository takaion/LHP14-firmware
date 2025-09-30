// Copyright 2025 Neo Trinity
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "joystick.h"
#include "analog.h"

// Joystick configurations
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

// Enable/disable stick (Buttons are always enabled)
static bool is_js_enabled = true;


// Layers
// Max 32 layers available
#define MAIN 0
#define NUMPADS 1
#define TEST 2


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
    }
    return true;
};

void matrix_scan_user(void) {
    if (!is_js_enabled) {
        joystick_set_axis(0, JS_X_MED); // X軸
        joystick_set_axis(1, JS_Y_MED); // Y軸
        joystick_flush();
    }
}

joystick_config_t joystick_axes[JOYSTICK_AXIS_COUNT] = {
    [0] = JOYSTICK_AXIS_IN(JS_PIN_X, JS_X_MAX, JS_X_MED, JS_X_MIN),
    [1] = JOYSTICK_AXIS_IN(JS_PIN_Y, JS_Y_MIN, JS_Y_MED, JS_Y_MAX),
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
}

void render_layer_name(const char* name) {
    oled_set_cursor(0, 2);
    oled_write_P(PSTR("Layer: "), false);
    oled_write_ln_P(name, false);
}

void render_layer(void) {
    switch (get_highest_layer(layer_state)) {
        case MAIN:
            render_layer_name(PSTR("MAIN"));
            rgblight_sethsv(0, 255, 90);
            break;
        case NUMPADS:
            render_layer_name(PSTR("NUMPADS"));
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
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, JS_0, TO(NUMPADS) \
    ),

    [NUMPADS] = LAYOUT( \
        KC_NUM,  KC_P7, KC_P8,       KC_P9,   LSFT(KC_MINS), \
        KC_PSLS, KC_P4, KC_P5,       KC_P6,   KC_PMNS, \
        KC_PAST, KC_P1, KC_P2,       KC_P3,   KC_PPLS, \
        KC_BSPC, KC_P0, DOUBLE_ZERO, KC_PDOT, KC_PENT, JS_0, TO(TEST)
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
