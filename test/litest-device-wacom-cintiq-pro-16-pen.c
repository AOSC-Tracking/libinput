/*
 * Copyright © 2019 Red Hat, Inc.
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

#include "config.h"

#include "litest-int.h"
#include "litest.h"

static struct input_event proximity_in[] = {
	{ .type = EV_ABS, .code = ABS_X, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_Y, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_Z, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_WHEEL, .value = 0 },
	{ .type = EV_ABS, .code = ABS_PRESSURE, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_DISTANCE, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_TILT_X, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_TILT_Y, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_MISC, .value = 2083 },
	{ .type = EV_MSC, .code = MSC_SERIAL, .value = 297797542 },
	{ .type = EV_KEY, .code = LITEST_BTN_TOOL_AUTO, .value = 1 },
	{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	{ .type = -1, .code = -1 },
};

static struct input_event proximity_out[] = {
	{ .type = EV_ABS, .code = ABS_X, .value = 0 },
	{ .type = EV_ABS, .code = ABS_Y, .value = 0 },
	{ .type = EV_ABS, .code = ABS_Z, .value = 0 },
	{ .type = EV_ABS, .code = ABS_WHEEL, .value = 0 },
	{ .type = EV_ABS, .code = ABS_PRESSURE, .value = 0 },
	{ .type = EV_ABS, .code = ABS_DISTANCE, .value = 0 },
	{ .type = EV_ABS, .code = ABS_TILT_X, .value = 0 },
	{ .type = EV_ABS, .code = ABS_TILT_Y, .value = 0 },
	{ .type = EV_ABS, .code = ABS_MISC, .value = 0 },
	{ .type = EV_MSC, .code = MSC_SERIAL, .value = 297797542 },
	{ .type = EV_KEY, .code = LITEST_BTN_TOOL_AUTO, .value = 0 },
	{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	{ .type = -1, .code = -1 },
};

static struct input_event motion[] = {
	{ .type = EV_ABS, .code = ABS_X, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_Y, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_Z, .value = 0 },
	{ .type = EV_ABS, .code = ABS_WHEEL, .value = 0 },
	{ .type = EV_ABS, .code = ABS_DISTANCE, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_PRESSURE, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_TILT_X, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_TILT_Y, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_MSC, .code = MSC_SERIAL, .value = 297797542 },
	{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	{ .type = -1, .code = -1 },
};

static int
get_axis_default(struct litest_device *d, unsigned int evcode, int32_t *value)
{
	switch (evcode) {
	case ABS_Z:
	case ABS_TILT_X:
	case ABS_TILT_Y:
		*value = 0;
		return 0;
	case ABS_PRESSURE:
		*value = 100;
		return 0;
	case ABS_DISTANCE:
		*value = 0;
		return 0;
	}
	return 1;
}

static struct litest_device_interface interface = {
	.tablet_proximity_in_events = proximity_in,
	.tablet_proximity_out_events = proximity_out,
	.tablet_motion_events = motion,

	.get_axis_default = get_axis_default,
};

/* clang-format off */
static struct input_absinfo absinfo[] = {
	{ ABS_X, 0, 69920, 0, 0, 200 },
	{ ABS_Y, 0, 39980, 0, 0, 200 },
	{ ABS_Z, -900, 899, 0, 0, 287 },
	{ ABS_WHEEL, 0, 2047, 0, 0, 0 },
	{ ABS_PRESSURE, 0, 8191, 0, 0, 0 },
	{ ABS_DISTANCE, 0, 63, 1, 0, 0 },
	{ ABS_TILT_X, -64, 63, 1, 0, 57 },
	{ ABS_TILT_Y, -64, 63, 1, 0, 57 },
	{ ABS_MISC, 0, 0, 0, 0, 0 },
	{ .value = -1 },
};
/* clang-format on */

static struct input_id input_id = {
	.bustype = 0x3,
	.vendor = 0x56a,
	.product = 0x350,
	.version = 0xb,
};

/* clang-format off */
static int events[] = {
	EV_KEY, BTN_TOOL_PEN,
	EV_KEY, BTN_TOOL_RUBBER,
	EV_KEY, BTN_TOOL_BRUSH,
	EV_KEY, BTN_TOOL_PENCIL,
	EV_KEY, BTN_TOOL_AIRBRUSH,
	EV_KEY, BTN_TOUCH,
	EV_KEY, BTN_STYLUS,
	EV_KEY, BTN_STYLUS2,
	EV_KEY, BTN_STYLUS3,
	EV_MSC, MSC_SERIAL,
	INPUT_PROP_MAX, INPUT_PROP_DIRECT,
	-1, -1,
};
/* clang-format on */

TEST_DEVICE(LITEST_WACOM_CINTIQ_PRO16_PEN,
	    .features = LITEST_TABLET | LITEST_DISTANCE | LITEST_TOOL_SERIAL |
			LITEST_TILT | LITEST_DIRECT | LITEST_HOVER,
	    .interface = &interface,

	    .name = "Wacom Cintiq Pro 16 Pen",
	    .id = &input_id,
	    .events = events,
	    .absinfo = absinfo,
	    .udev_properties = {
		    { "LIBINPUT_DEVICE_GROUP", "wacom-pro16-group" },
		    { NULL },
	    }, )
