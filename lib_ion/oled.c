// OLED の描画処理を記述
#include QMK_KEYBOARD_H
#include "lib_ion/joystick.h"
#include "oled.h"

void render_logo(void) {
    for (uint8_t i = 0; i < LOGO_LINES; i++) {
        oled_set_cursor(0, i);
        oled_write_P(lhp_logo[i], false);
    }
};

void render_layer_name(const char* name) {
    oled_write_P(PSTR("Layer: "), false);
    oled_write_ln_P(name, false);
}

void render_lock_state(void) {
    led_t led_state = host_keyboard_led_state();
    oled_write_P(led_state.num_lock ? PSTR("NL ") : PSTR("   "), false);
    oled_write_P(led_state.caps_lock ? PSTR("CL ") : PSTR("   "), false);
    oled_write_P(led_state.scroll_lock ? PSTR("SL") : PSTR("  "), false);
}

void render_js_state(struct JOYSTICK_STATE *js_state, struct JOYSTICK_RAPID_STATE *js_rapid_state, bool js_is_mouse) {
    oled_write_P(PSTR("JS:"), false);
    oled_write_P(js_state->enabled ? (js_is_mouse ? PSTR("M") : PSTR("E")) : PSTR("D"), false);
    oled_write_P(js_rapid_state->enabled ? PSTR("R") : PSTR("-"), false);
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

void render_joystick_angles(struct JOYSTICK_STATE *state) {
    oled_write_P(PSTR("X:"), false);
    render_joystick_angle(state->x);
    oled_write_P(PSTR(" Y:"), false);
    render_joystick_angle(state->y);
}
