/*
 * Copyright © 2014 Red Hat, Inc.
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

#include <assert.h>

#include "litest-int.h"
#include "litest.h"

static bool
touch_down(struct litest_device *d, unsigned int slot, double x, double y)
{
	assert(slot == 0);

	litest_event(d, EV_ABS, ABS_X, litest_scale(d, ABS_X, x));
	litest_event(d, EV_ABS, ABS_Y, litest_scale(d, ABS_Y, y));
	litest_event(d, EV_SYN, SYN_REPORT, 0);

	return true; /* we handled the event */
}

static bool
touch_move(struct litest_device *d, unsigned int slot, double x, double y)
{
	assert(slot == 0);

	litest_event(d, EV_ABS, ABS_X, litest_scale(d, ABS_X, x));
	litest_event(d, EV_ABS, ABS_Y, litest_scale(d, ABS_Y, y));
	litest_event(d, EV_SYN, SYN_REPORT, 0);

	return true; /* we handled the event */
}

static bool
touch_up(struct litest_device *d, unsigned int slot)
{
	assert(slot == 0);
	litest_event(d, EV_SYN, SYN_REPORT, 0);

	return true; /* we handled the event */
}

static struct litest_device_interface interface = {
	.touch_down = touch_down,
	.touch_move = touch_move,
	.touch_up = touch_up,
};

/* clang-format off */
static struct input_absinfo absinfo[] = {
	{ ABS_X, 0, 32767, 0, 0, 0 },
	{ ABS_Y, 0, 32767, 0, 0, 0 },
	{ .value = -1 },
};
/* clang-format on */

static struct input_id input_id = {
	.bustype = 0x03,
	.vendor = 0xe0f,
	.product = 0x03,
};

/* clang-format off */
static int events[] = {
	EV_KEY, BTN_LEFT,
	EV_KEY, BTN_RIGHT,
	EV_KEY, BTN_MIDDLE,
	EV_KEY, BTN_SIDE,
	EV_KEY, BTN_EXTRA,
	EV_KEY, BTN_FORWARD,
	EV_KEY, BTN_BACK,
	EV_KEY, BTN_TASK,
	EV_KEY, 280,
	EV_KEY, 281,
	EV_KEY, 282,
	EV_KEY, 283,
	EV_KEY, 284,
	EV_KEY, 285,
	EV_KEY, 286,
	EV_KEY, 287,
	EV_REL, REL_WHEEL,
	EV_REL, REL_HWHEEL,
	-1, -1,
};
/* clang-format on */

TEST_DEVICE(LITEST_VMWARE_VIRTMOUSE,
	    .features = LITEST_WHEEL | LITEST_BUTTON | LITEST_ABSOLUTE |
			LITEST_NO_DEBOUNCE,
	    .interface = &interface,

	    .name = "VMware VMware Virtual USB Mouse",
	    .id = &input_id,
	    .events = events,
	    .absinfo = absinfo, )
