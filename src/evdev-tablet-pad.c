/*
 * Copyright © 2016 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"
#include "evdev-tablet-pad.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define pad_set_status(pad_,s_) (pad_)->status |= (s_)
#define pad_unset_status(pad_,s_) (pad_)->status &= ~(s_)
#define pad_has_status(pad_,s_) (!!((pad_)->status & (s_)))

static void
pad_get_buttons_pressed(struct pad_dispatch *pad,
			struct button_state *buttons)
{
	struct button_state *state = &pad->button_state;
	struct button_state *prev_state = &pad->prev_button_state;
	unsigned int i;

	for (i = 0; i < sizeof(buttons->bits); i++)
		buttons->bits[i] = state->bits[i] & ~(prev_state->bits[i]);
}

static void
pad_get_buttons_released(struct pad_dispatch *pad,
			 struct button_state *buttons)
{
	struct button_state *state = &pad->button_state;
	struct button_state *prev_state = &pad->prev_button_state;
	unsigned int i;

	for (i = 0; i < sizeof(buttons->bits); i++)
		buttons->bits[i] = prev_state->bits[i] & ~(state->bits[i]);
}

static inline bool
pad_button_is_down(const struct pad_dispatch *pad,
		   uint32_t button)
{
	return bit_is_set(pad->button_state.bits, button);
}

static inline bool
pad_any_button_down(const struct pad_dispatch *pad)
{
	const struct button_state *state = &pad->button_state;
	unsigned int i;

	for (i = 0; i < sizeof(state->bits); i++)
		if (state->bits[i] != 0)
			return true;

	return false;
}

static inline void
pad_button_set_down(struct pad_dispatch *pad,
		    uint32_t button,
		    bool is_down)
{
	struct button_state *state = &pad->button_state;

	if (is_down) {
		set_bit(state->bits, button);
		pad_set_status(pad, PAD_BUTTONS_PRESSED);
	} else {
		clear_bit(state->bits, button);
		pad_set_status(pad, PAD_BUTTONS_RELEASED);
	}
}

static void
pad_process_absolute(struct pad_dispatch *pad,
		     struct evdev_device *device,
		     struct input_event *e,
		     uint64_t time)
{
	switch (e->code) {
	case ABS_WHEEL:
		pad->changed_axes |= PAD_AXIS_RING1;
		pad_set_status(pad, PAD_AXES_UPDATED);
		break;
	case ABS_THROTTLE:
		pad->changed_axes |= PAD_AXIS_RING2;
		pad_set_status(pad, PAD_AXES_UPDATED);
		break;
	case ABS_RX:
		pad->changed_axes |= PAD_AXIS_STRIP1;
		pad_set_status(pad, PAD_AXES_UPDATED);
		break;
	case ABS_RY:
		pad->changed_axes |= PAD_AXIS_STRIP2;
		pad_set_status(pad, PAD_AXES_UPDATED);
		break;
	case ABS_MISC:
		/* The wacom driver always sends a 0 axis event on finger
		   up, but we also get an ABS_MISC 15 on touch down and
		   ABS_MISC 0 on touch up, on top of the actual event. This
		   is kernel behavior for xf86-input-wacom backwards
		   compatibility after the 3.17 wacom HID move.

		   We use that event to tell when we truly went a full
		   rotation around the wheel vs. a finger release.

		   FIXME: On the Intuos5 and later the kernel merges all
		   states into that event, so if any finger is down on any
		   button, the wheel release won't trigger the ABS_MISC 0
		   but still send a 0 event. We can't currently detect this.
		 */
		pad->have_abs_misc_terminator = true;
		break;
	default:
		log_info(device->base.seat->libinput,
			 "Unhandled EV_ABS event code %#x\n", e->code);
		break;
	}
}

static inline double
normalize_ring(const struct input_absinfo *absinfo)
{
	/* libinput has 0 as the ring's northernmost point in the device's
	   current logical rotation, increasing clockwise to 1. Wacom has
	   0 on the left-most wheel position.
	 */
	double range = absinfo->maximum - absinfo->minimum + 1;
	double value = (absinfo->value - absinfo->minimum) / range - 0.25;

	if (value < 0.0)
		value += 1.0;

	return value;
}

static inline double
normalize_strip(const struct input_absinfo *absinfo)
{
	/* strip axes don't use a proper value, they just shift the bit left
	 * for each position. 0 isn't a real value either, it's only sent on
	 * finger release */
	double min = 0,
	       max = log2(absinfo->maximum);
	double range = max - min;
	double value = (log2(absinfo->value) - min) / range;

	return value;
}

static inline double
pad_handle_ring(struct pad_dispatch *pad,
		struct evdev_device *device,
		unsigned int code)
{
	const struct input_absinfo *absinfo;
	double degrees;

	absinfo = libevdev_get_abs_info(device->evdev, code);
	assert(absinfo);

	degrees = normalize_ring(absinfo) * 360;

	if (device->left_handed.enabled)
		degrees = fmod(degrees + 180, 360);

	return degrees;
}

static inline double
pad_handle_strip(struct pad_dispatch *pad,
		 struct evdev_device *device,
		 unsigned int code)
{
	const struct input_absinfo *absinfo;
	double pos;

	absinfo = libevdev_get_abs_info(device->evdev, code);
	assert(absinfo);

	if (absinfo->value == 0)
		return 0.0;

	pos = normalize_strip(absinfo);

	if (device->left_handed.enabled)
		pos = 1.0 - pos;

	return pos;
}

static void
pad_check_notify_axes(struct pad_dispatch *pad,
		      struct evdev_device *device,
		      uint64_t time)
{
	struct libinput_device *base = &device->base;
	double value;
	bool send_finger_up = false;

	/* Suppress the reset to 0 on finger up. See the
	   comment in pad_process_absolute */
	if (pad->have_abs_misc_terminator &&
	    libevdev_get_event_value(device->evdev, EV_ABS, ABS_MISC) == 0)
		send_finger_up = true;

	if (pad->changed_axes & PAD_AXIS_RING1) {
		value = pad_handle_ring(pad, device, ABS_WHEEL);
		if (send_finger_up)
			value = -1.0;

		tablet_pad_notify_ring(base,
				       time,
				       0,
				       value,
				       LIBINPUT_TABLET_PAD_RING_SOURCE_FINGER);
	}

	if (pad->changed_axes & PAD_AXIS_RING2) {
		value = pad_handle_ring(pad, device, ABS_THROTTLE);
		if (send_finger_up)
			value = -1.0;

		tablet_pad_notify_ring(base,
				       time,
				       1,
				       value,
				       LIBINPUT_TABLET_PAD_RING_SOURCE_FINGER);
	}

	if (pad->changed_axes & PAD_AXIS_STRIP1) {
		value = pad_handle_strip(pad, device, ABS_RX);
		if (send_finger_up)
			value = -1.0;

		tablet_pad_notify_strip(base,
					time,
					0,
					value,
					LIBINPUT_TABLET_PAD_STRIP_SOURCE_FINGER);
	}

	if (pad->changed_axes & PAD_AXIS_STRIP2) {
		value = pad_handle_strip(pad, device, ABS_RY);
		if (send_finger_up)
			value = -1.0;

		tablet_pad_notify_strip(base,
					time,
					1,
					value,
					LIBINPUT_TABLET_PAD_STRIP_SOURCE_FINGER);
	}

	pad->changed_axes = PAD_AXIS_NONE;
	pad->have_abs_misc_terminator = false;
}

static void
pad_process_key(struct pad_dispatch *pad,
		struct evdev_device *device,
		struct input_event *e,
		uint64_t time)
{
	uint32_t button = e->code;
	uint32_t is_press = e->value != 0;

	pad_button_set_down(pad, button, is_press);
}

static void
pad_notify_button_mask(struct pad_dispatch *pad,
		       struct evdev_device *device,
		       uint64_t time,
		       const struct button_state *buttons,
		       enum libinput_button_state state)
{
	struct libinput_device *base = &device->base;
	int32_t code;
	unsigned int i;

	for (i = 0; i < sizeof(buttons->bits); i++) {
		unsigned char buttons_slice = buttons->bits[i];

		code = i * 8;
		while (buttons_slice) {
			int enabled;
			char map;

			code++;
			enabled = (buttons_slice & 1);
			buttons_slice >>= 1;

			if (!enabled)
				continue;

			map = pad->button_map[code - 1];
			if (map != -1)
				tablet_pad_notify_button(base, time, map, state);
		}
	}
}

static void
pad_notify_buttons(struct pad_dispatch *pad,
		   struct evdev_device *device,
		   uint64_t time,
		   enum libinput_button_state state)
{
	struct button_state buttons;

	if (state == LIBINPUT_BUTTON_STATE_PRESSED)
		pad_get_buttons_pressed(pad, &buttons);
	else
		pad_get_buttons_released(pad, &buttons);

	pad_notify_button_mask(pad, device, time, &buttons, state);
}

static void
pad_change_to_left_handed(struct evdev_device *device)
{
	struct pad_dispatch *pad = (struct pad_dispatch*)device->dispatch;

	if (device->left_handed.enabled == device->left_handed.want_enabled)
		return;

	if (pad_any_button_down(pad))
		return;

	device->left_handed.enabled = device->left_handed.want_enabled;
}

static void
pad_flush(struct pad_dispatch *pad,
	  struct evdev_device *device,
	  uint64_t time)
{
	if (pad_has_status(pad, PAD_AXES_UPDATED)) {
		pad_check_notify_axes(pad, device, time);
		pad_unset_status(pad, PAD_AXES_UPDATED);
	}

	if (pad_has_status(pad, PAD_BUTTONS_RELEASED)) {
		pad_notify_buttons(pad,
				   device,
				   time,
				   LIBINPUT_BUTTON_STATE_RELEASED);
		pad_unset_status(pad, PAD_BUTTONS_RELEASED);

		pad_change_to_left_handed(device);
	}

	if (pad_has_status(pad, PAD_BUTTONS_PRESSED)) {
		pad_notify_buttons(pad,
				   device,
				   time,
				   LIBINPUT_BUTTON_STATE_PRESSED);
		pad_unset_status(pad, PAD_BUTTONS_PRESSED);
	}

	/* Update state */
	memcpy(&pad->prev_button_state,
	       &pad->button_state,
	       sizeof(pad->button_state));
}

static void
pad_process(struct evdev_dispatch *dispatch,
	    struct evdev_device *device,
	    struct input_event *e,
	    uint64_t time)
{
	struct pad_dispatch *pad = (struct pad_dispatch *)dispatch;

	switch (e->type) {
	case EV_ABS:
		pad_process_absolute(pad, device, e, time);
		break;
	case EV_KEY:
		pad_process_key(pad, device, e, time);
		break;
	case EV_SYN:
		pad_flush(pad, device, time);
		break;
	case EV_MSC:
		/* The EKR sends the serial as MSC_SERIAL, ignore this for
		 * now */
		break;
	default:
		log_error(device->base.seat->libinput,
			  "Unexpected event type %s (%#x)\n",
			  libevdev_event_type_get_name(e->type),
			  e->type);
		break;
	}
}

static void
pad_suspend(struct evdev_dispatch *dispatch,
	    struct evdev_device *device)
{
	struct pad_dispatch *pad = (struct pad_dispatch *)dispatch;
	struct libinput *libinput = device->base.seat->libinput;
	unsigned int code;

	for (code = KEY_ESC; code < KEY_CNT; code++) {
		if (pad_button_is_down(pad, code))
			pad_button_set_down(pad, code, false);
	}

	pad_flush(pad, device, libinput_now(libinput));
}

static void
pad_destroy(struct evdev_dispatch *dispatch)
{
	struct pad_dispatch *pad = (struct pad_dispatch*)dispatch;

	free(pad);
}

static struct evdev_dispatch_interface pad_interface = {
	pad_process,
	pad_suspend, /* suspend */
	NULL, /* remove */
	pad_destroy,
	NULL, /* device_added */
	NULL, /* device_removed */
	NULL, /* device_suspended */
	NULL, /* device_resumed */
	NULL, /* post_added */
};

static void
pad_init_buttons(struct pad_dispatch *pad,
		 struct evdev_device *device)
{
	unsigned int code;
	size_t i;
	int map = 0;

	for (i = 0; i < ARRAY_LENGTH(pad->button_map); i++)
		pad->button_map[i] = -1;

	/* we match wacom_report_numbered_buttons() from the kernel */
	for (code = BTN_0; code < BTN_0 + 10; code++) {
		if (libevdev_has_event_code(device->evdev, EV_KEY, code))
			pad->button_map[code] = map++;
	}

	for (code = BTN_BASE; code < BTN_BASE + 2; code++) {
		if (libevdev_has_event_code(device->evdev, EV_KEY, code))
			pad->button_map[code] = map++;
	}

	for (code = BTN_A; code < BTN_A + 6; code++) {
		if (libevdev_has_event_code(device->evdev, EV_KEY, code))
			pad->button_map[code] = map++;
	}

	pad->nbuttons = map;
}

static void
pad_init_left_handed(struct evdev_device *device)
{
	if (evdev_tablet_has_left_handed(device))
		evdev_init_left_handed(device,
				       pad_change_to_left_handed);
}

static int
pad_init(struct pad_dispatch *pad, struct evdev_device *device)
{
	pad->base.interface = &pad_interface;
	pad->device = device;
	pad->status = PAD_NONE;
	pad->changed_axes = PAD_AXIS_NONE;

	pad_init_buttons(pad, device);
	pad_init_left_handed(device);

	return 0;
}

static uint32_t
pad_sendevents_get_modes(struct libinput_device *device)
{
	return LIBINPUT_CONFIG_SEND_EVENTS_DISABLED;
}

static enum libinput_config_status
pad_sendevents_set_mode(struct libinput_device *device,
			enum libinput_config_send_events_mode mode)
{
	struct evdev_device *evdev = (struct evdev_device*)device;
	struct pad_dispatch *pad = (struct pad_dispatch*)evdev->dispatch;

	if (mode == pad->sendevents.current_mode)
		return LIBINPUT_CONFIG_STATUS_SUCCESS;

	switch(mode) {
	case LIBINPUT_CONFIG_SEND_EVENTS_ENABLED:
		break;
	case LIBINPUT_CONFIG_SEND_EVENTS_DISABLED:
		pad_suspend(evdev->dispatch, evdev);
		break;
	default:
		return LIBINPUT_CONFIG_STATUS_UNSUPPORTED;
	}

	pad->sendevents.current_mode = mode;

	return LIBINPUT_CONFIG_STATUS_SUCCESS;
}

static enum libinput_config_send_events_mode
pad_sendevents_get_mode(struct libinput_device *device)
{
	struct evdev_device *evdev = (struct evdev_device*)device;
	struct pad_dispatch *dispatch = (struct pad_dispatch*)evdev->dispatch;

	return dispatch->sendevents.current_mode;
}

static enum libinput_config_send_events_mode
pad_sendevents_get_default_mode(struct libinput_device *device)
{
	return LIBINPUT_CONFIG_SEND_EVENTS_ENABLED;
}

struct evdev_dispatch *
evdev_tablet_pad_create(struct evdev_device *device)
{
	struct pad_dispatch *pad;

	pad = zalloc(sizeof *pad);
	if (!pad)
		return NULL;

	if (pad_init(pad, device) != 0) {
		pad_destroy(&pad->base);
		return NULL;
	}

	device->base.config.sendevents = &pad->sendevents.config;
	pad->sendevents.current_mode = LIBINPUT_CONFIG_SEND_EVENTS_ENABLED;
	pad->sendevents.config.get_modes = pad_sendevents_get_modes;
	pad->sendevents.config.set_mode = pad_sendevents_set_mode;
	pad->sendevents.config.get_mode = pad_sendevents_get_mode;
	pad->sendevents.config.get_default_mode = pad_sendevents_get_default_mode;

	return &pad->base;
}

int
evdev_device_tablet_pad_get_num_buttons(struct evdev_device *device)
{
	struct pad_dispatch *pad = (struct pad_dispatch*)device->dispatch;

	if (!(device->seat_caps & EVDEV_DEVICE_TABLET_PAD))
		return -1;

	return pad->nbuttons;
}

int
evdev_device_tablet_pad_get_num_rings(struct evdev_device *device)
{
	int nrings = 0;

	if (!(device->seat_caps & EVDEV_DEVICE_TABLET_PAD))
		return -1;

	if (libevdev_has_event_code(device->evdev, EV_ABS, ABS_WHEEL)) {
		nrings++;
		if (libevdev_has_event_code(device->evdev,
					    EV_ABS,
					    ABS_THROTTLE))
			nrings++;
	}

	return nrings;
}

int
evdev_device_tablet_pad_get_num_strips(struct evdev_device *device)
{
	int nstrips = 0;

	if (!(device->seat_caps & EVDEV_DEVICE_TABLET_PAD))
		return -1;

	if (libevdev_has_event_code(device->evdev, EV_ABS, ABS_RX)) {
		nstrips++;
		if (libevdev_has_event_code(device->evdev,
					    EV_ABS,
					    ABS_RY))
			nstrips++;
	}

	return nstrips;
}
