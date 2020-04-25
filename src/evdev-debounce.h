/*
 * Copyright © 2020 Jacob Moroni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef EVDEV_DEBOUNCE_H
#define EVDEV_DEBOUNCE_H

#include "evdev.h"

enum debounce_state {
	DEBOUNCE_STATE_IS_UP = 100,
	DEBOUNCE_STATE_IS_DOWN,
	DEBOUNCE_STATE_IS_DOWN_WAITING,
	DEBOUNCE_STATE_IS_UP_DELAYING,
	DEBOUNCE_STATE_IS_UP_DELAYING_SPURIOUS,
	DEBOUNCE_STATE_IS_UP_DETECTING_SPURIOUS,
	DEBOUNCE_STATE_IS_DOWN_DETECTING_SPURIOUS,
	DEBOUNCE_STATE_IS_UP_WAITING,
	DEBOUNCE_STATE_IS_DOWN_DELAYING,

	DEBOUNCE_STATE_DISABLED = 999,
};

struct debounce {
	struct evdev_device *device;
	const struct debounce_key_ops *key_ops;
	unsigned int button_code;
	uint64_t button_time;
	struct libinput_timer timer;
	struct libinput_timer timer_short;
	enum debounce_state state;
	bool spurious_enabled;
};

enum key_type {
	KEY_TYPE_NONE,
	KEY_TYPE_KEY,
	KEY_TYPE_BUTTON,
};

struct debounce_key_ops {
	bool (*key_has_changed)(struct evdev_device *device, int code);
	bool (*is_key_down)(struct evdev_device *device, int code);
};

static inline enum key_type
get_key_type(uint16_t code)
{
	switch (code) {
	case BTN_TOOL_PEN:
	case BTN_TOOL_RUBBER:
	case BTN_TOOL_BRUSH:
	case BTN_TOOL_PENCIL:
	case BTN_TOOL_AIRBRUSH:
	case BTN_TOOL_MOUSE:
	case BTN_TOOL_LENS:
	case BTN_TOOL_QUINTTAP:
	case BTN_TOOL_DOUBLETAP:
	case BTN_TOOL_TRIPLETAP:
	case BTN_TOOL_QUADTAP:
	case BTN_TOOL_FINGER:
	case BTN_TOUCH:
		return KEY_TYPE_NONE;
	}

	if (code >= KEY_ESC && code <= KEY_MICMUTE)
		return KEY_TYPE_KEY;
	if (code >= BTN_MISC && code <= BTN_GEAR_UP)
		return KEY_TYPE_BUTTON;
	if (code >= KEY_OK && code <= KEY_LIGHTS_TOGGLE)
		return KEY_TYPE_KEY;
	if (code >= BTN_DPAD_UP && code <= BTN_DPAD_RIGHT)
		return KEY_TYPE_BUTTON;
	if (code >= KEY_ALS_TOGGLE && code <= KEY_ONSCREEN_KEYBOARD)
		return KEY_TYPE_KEY;
	if (code >= BTN_TRIGGER_HAPPY && code <= BTN_TRIGGER_HAPPY40)
		return KEY_TYPE_BUTTON;
	return KEY_TYPE_NONE;
}

void
init_debounce(struct debounce *debounce,
	      struct evdev_device *device,
	      const struct debounce_key_ops *key_ops);

void
debounce_handle_state(struct debounce *debounce, uint64_t time);

#endif
