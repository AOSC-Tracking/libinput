/*
 * Copyright © 2016 Red Hat, Inc.
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

static struct input_event down[] = {
	{ .type = -1, .code = -1 },
};

static struct input_event move[] = {
	{ .type = -1, .code = -1 },
};

static struct input_event strip_start[] = {
	{ .type = EV_ABS, .code = ABS_RX, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_ABS, .code = ABS_MISC, .value = 15 },
	{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	{ .type = -1, .code = -1 },
};

static struct input_event strip_change[] = {
	{ .type = EV_ABS, .code = ABS_RX, .value = LITEST_AUTO_ASSIGN },
	{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	{ .type = -1, .code = -1 },
};

static struct input_event strip_end[] = {
	{ .type = EV_ABS, .code = ABS_RX, .value = 0 },
	{ .type = EV_ABS, .code = ABS_MISC, .value = 0 },
	{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	{ .type = -1, .code = -1 },
};

static struct litest_device_interface interface = {
	.touch_down_events = down,
	.touch_move_events = move,
	.pad_strip_start_events = strip_start,
	.pad_strip_change_events = strip_change,
	.pad_strip_end_events = strip_end,
};

/* clang-format off */
static struct input_absinfo absinfo[] = {
	{ ABS_X, 0, 1, 0, 0, 0 },
	{ ABS_Y, 0, 1, 0, 0, 0 },
	{ ABS_RX, 0, 4096, 0, 0, 0 },
	{ ABS_MISC, 0, 0, 0, 0, 0 },
	{ .value = -1 },
};
/* clang-format on */

static struct input_id input_id = {
	.bustype = 0x3,
	.vendor = 0x56a,
	.product = 0xb7,
};

/* clang-format off */
static int events[] = {
	EV_KEY, BTN_0,
	EV_KEY, BTN_1,
	EV_KEY, BTN_2,
	EV_KEY, BTN_3,
	EV_KEY, BTN_STYLUS,
	-1, -1,
};
/* clang-format on */

TEST_DEVICE(LITEST_WACOM_INTUOS3_PAD,
	    .features = LITEST_TABLET_PAD | LITEST_STRIP,
	    .interface = &interface,

	    .name = "Wacom Intuos3 4x6 Pad",
	    .id = &input_id,
	    .events = events,
	    .absinfo = absinfo,
	    .udev_properties = {
		    { "ID_INPUT_TABLET_PAD", "1" },
		    { NULL },
	    }, )
