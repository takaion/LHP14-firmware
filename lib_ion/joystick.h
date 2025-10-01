#pragma once

// Debug mode: show the joystick angles to OLED (comment out or undef this to disable)
#define JS_DEBUG_ENABLED

#define JS_DEFAULT_ENABLED true
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

#define JS_RAPID_INTERVAL 60
// 0 (Fastest) - 127 (Slowest)
#define JS_MOUSE_SPEED 20

struct JOYSTICK_ANGLES { int16_t x; int16_t y; };
struct JOYSTICK_STATE {
    int16_t x;
    int16_t y;
    bool enabled;
};
struct JOYSTICK_RAPID_STATE {
    uint8_t button;
    bool enabled;
    bool pressing;
    uint16_t timer;
};

#define JS_INIT {0, 0, JS_DEFAULT_ENABLED}
bool is_in_deadzone(int16_t x, int16_t y, uint16_t dz);
int16_t joystick_angle(int16_t raw, int16_t min, int16_t mid, int16_t max);
void read_joystick_angles(struct JOYSTICK_STATE *state);
void report_joystick(struct JOYSTICK_STATE *state, uint8_t x_axis, uint8_t y_axis);
void report_joystick_as_mouse(struct JOYSTICK_STATE *js_state);

#define JS_RAPID_INIT(B) {B, false, false, 0}
void start_joystick_rapid(struct JOYSTICK_RAPID_STATE *state);
void stop_joystick_rapid(struct JOYSTICK_RAPID_STATE *state);
void toggle_joystick_rapid(struct JOYSTICK_RAPID_STATE *state);
void run_joystick_rapid(struct JOYSTICK_RAPID_STATE *state);
