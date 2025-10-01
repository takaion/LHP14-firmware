#pragma once
#include "lib_ion/joystick.h"

void render_logo(void);
void render_layer_name(const char* name);
void render_lock_state(void);
void render_js_state(struct JOYSTICK_STATE *js_state, struct JOYSTICK_RAPID_STATE *js_rapid_state, bool js_is_mouse);
void render_joystick_angle(int16_t val);
void render_joystick_angles(struct JOYSTICK_STATE *angles);
