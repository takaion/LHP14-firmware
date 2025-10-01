// Copyright 2025 Neo Trinity
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "lib_ion/oled.h"
#include "lib_ion/joystick.h"

// Button repeating
#define JS_RAPID_BUTTON 1

// Joystick configurations
// Enable/disable stick (Buttons are always enabled)
static struct JOYSTICK_STATE js_state = JS_INIT;
static bool is_joystick_mouse = false;
static struct JOYSTICK_RAPID_STATE js_rapid_state = JS_RAPID_INIT(JS_RAPID_BUTTON);

// Layers
// Max 32 layers available
#define MAIN 0
#define NUMPADS 1
#define FUNCTIONS 2
#define TEST 3

enum custom_keycodes {
    RGBRST = SAFE_RANGE,
    DOUBLE_ZERO,
    JS_TOGGLE,
    JS_RAPID,
    JS_MO_TOGGLE,
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
                js_state.enabled = !js_state.enabled;
            }
            break;
        case JS_RAPID:
            if (record->event.pressed) {
                toggle_joystick_rapid(&js_rapid_state);
            }
            break;
        case JS_MO_TOGGLE:
            if (record->event.pressed) {
                is_joystick_mouse = !is_joystick_mouse;
            }
            break;
    }
    return true;
};

void matrix_scan_user(void) {
    run_joystick_rapid(&js_rapid_state);
    read_joystick_angles(&js_state);
    if (!is_joystick_mouse) report_joystick(&js_state, 0, 1);
    else report_joystick_as_mouse(&js_state);
}

joystick_config_t joystick_axes[JOYSTICK_AXIS_COUNT] = {
    JOYSTICK_AXIS_VIRTUAL,
    JOYSTICK_AXIS_VIRTUAL,
};

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
    oled_set_cursor(13, 0);
    render_lock_state();
    oled_set_cursor(13, 1);
    render_js_state(&js_state, &js_rapid_state, is_joystick_mouse);
    oled_set_cursor(0, 2);
    render_layer();
    #ifdef JS_DEBUG_ENABLED
    oled_set_cursor(0, 3);
    render_joystick_angles(&js_state);
    #endif
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
        JS_MO_TOGGLE, MS_BTN4, MS_BTN5, LSA(KC_F9), JS_TOGGLE, \
        XXXXXXX,      MS_BTN2, MS_WHLU, MS_BTN1,    JS_RAPID, \
        XXXXXXX,      MS_WHLL, MS_WHLD, MS_WHLR,    KC_F13, \
        KC_MUTE,      XXXXXXX, MS_BTN3, XXXXXXX,    LALT(KC_MPLY), JS_0, TO(NUMPADS) \
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
