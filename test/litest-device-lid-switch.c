/*
 * Copyright © 2017 James Ye <jye836@gmail.com>
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

static struct input_id input_id = {
	.bustype = 0x19,
	.vendor = 0x0,
	.product = 0x5,
};

/* clang-format off */
static int events[] = {
	EV_SW, SW_LID,
	-1, -1,
};
/* clang-format on */

static const char quirk_file[] =
	"[litest Lid Switch]\n"
	"MatchName=litest Lid Switch\n"
	"AttrLidSwitchReliability=reliable\n";

TEST_DEVICE(LITEST_LID_SWITCH,
	    .features = LITEST_SWITCH,
	    .interface = NULL,

	    .name = "Lid Switch",
	    .id = &input_id,
	    .events = events,
	    .absinfo = NULL,

	    .quirk_file = quirk_file,
	    .udev_properties = {
		    { "ID_INPUT_SWITCH", "1" },
		    { NULL },
	    }, )
