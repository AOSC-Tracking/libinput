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

#ifndef _SHARED_H_
#define _SHARED_H_

#include "config.h"

#include <getopt.h>
#include <libinput.h>
#include <limits.h>
#include <quirks.h>
#include <stdbool.h>

#include "util-strings.h"

#define EXIT_INVALID_USAGE 2

extern uint32_t log_serial;

enum configuration_options {
	OPT_TAP_ENABLE = 256,
	OPT_TAP_DISABLE,
	OPT_TAP_MAP,
	OPT_DRAG_ENABLE,
	OPT_DRAG_DISABLE,
	OPT_DRAG_LOCK_ENABLE,
	OPT_DRAG_LOCK_DISABLE,
	OPT_NATURAL_SCROLL_ENABLE,
	OPT_NATURAL_SCROLL_DISABLE,
	OPT_LEFT_HANDED_ENABLE,
	OPT_LEFT_HANDED_DISABLE,
	OPT_MIDDLEBUTTON_ENABLE,
	OPT_MIDDLEBUTTON_DISABLE,
	OPT_DWT_ENABLE,
	OPT_DWT_DISABLE,
	OPT_DWTP_ENABLE,
	OPT_DWTP_DISABLE,
	OPT_CLICK_METHOD,
	OPT_CLICKFINGER_MAP,
	OPT_SCROLL_METHOD,
	OPT_SCROLL_BUTTON,
	OPT_SCROLL_BUTTON_LOCK_ENABLE,
	OPT_SCROLL_BUTTON_LOCK_DISABLE,
	OPT_SPEED,
	OPT_PROFILE,
	OPT_DISABLE_SENDEVENTS,
	OPT_APPLY_TO,
	OPT_CUSTOM_POINTS,
	OPT_CUSTOM_STEP,
	OPT_CUSTOM_TYPE,
	OPT_ROTATION_ANGLE,
	OPT_PRESSURE_RANGE,
	OPT_CALIBRATION,
	OPT_AREA,
	OPT_3FG_DRAG,
	OPT_SENDEVENTS,
	OPT_ERASER_BUTTON_MODE,
	OPT_ERASER_BUTTON_BUTTON,
	OPT_PLUGINS_DISABLE,
	OPT_PLUGINS_ENABLE,
	OPT_PLUGIN_PATH,
};

#define CONFIGURATION_OPTIONS \
	{ "disable-sendevents",        required_argument, 0, OPT_DISABLE_SENDEVENTS }, \
	{ "enable-plugins",            no_argument,       0, OPT_PLUGINS_ENABLE }, \
	{ "disable-plugins",           no_argument,       0, OPT_PLUGINS_DISABLE }, \
	{ "enable-tap",                no_argument,       0, OPT_TAP_ENABLE }, \
	{ "disable-tap",               no_argument,       0, OPT_TAP_DISABLE }, \
	{ "enable-drag",               no_argument,       0, OPT_DRAG_ENABLE }, \
	{ "disable-drag",              no_argument,       0, OPT_DRAG_DISABLE }, \
	{ "enable-drag-lock",          optional_argument, 0, OPT_DRAG_LOCK_ENABLE }, \
	{ "disable-drag-lock",         no_argument,       0, OPT_DRAG_LOCK_DISABLE }, \
	{ "enable-natural-scrolling",  no_argument,       0, OPT_NATURAL_SCROLL_ENABLE }, \
	{ "disable-natural-scrolling", no_argument,       0, OPT_NATURAL_SCROLL_DISABLE }, \
	{ "enable-left-handed",        no_argument,       0, OPT_LEFT_HANDED_ENABLE }, \
	{ "disable-left-handed",       no_argument,       0, OPT_LEFT_HANDED_DISABLE }, \
	{ "enable-middlebutton",       no_argument,       0, OPT_MIDDLEBUTTON_ENABLE }, \
	{ "disable-middlebutton",      no_argument,       0, OPT_MIDDLEBUTTON_DISABLE }, \
	{ "enable-dwt",                no_argument,       0, OPT_DWT_ENABLE }, \
	{ "disable-dwt",               no_argument,       0, OPT_DWT_DISABLE }, \
	{ "enable-dwtp",               no_argument,       0, OPT_DWTP_ENABLE }, \
	{ "disable-dwtp",              no_argument,       0, OPT_DWTP_DISABLE }, \
	{ "enable-scroll-button-lock", no_argument,       0, OPT_SCROLL_BUTTON_LOCK_ENABLE }, \
	{ "disable-scroll-button-lock",no_argument,       0, OPT_SCROLL_BUTTON_LOCK_DISABLE }, \
	{ "enable-3fg-drag",           required_argument, 0, OPT_3FG_DRAG }, \
	{ "set-click-method",          required_argument, 0, OPT_CLICK_METHOD }, \
	{ "set-clickfinger-map",       required_argument, 0, OPT_CLICKFINGER_MAP }, \
	{ "set-scroll-method",         required_argument, 0, OPT_SCROLL_METHOD }, \
	{ "set-scroll-button",         required_argument, 0, OPT_SCROLL_BUTTON }, \
	{ "set-profile",               required_argument, 0, OPT_PROFILE }, \
	{ "set-tap-map",               required_argument, 0, OPT_TAP_MAP }, \
	{ "set-speed",                 required_argument, 0, OPT_SPEED },\
	{ "set-sendevents",            required_argument, 0, OPT_SENDEVENTS },\
	{ "apply-to",                  required_argument, 0, OPT_APPLY_TO },\
	{ "set-custom-points",         required_argument, 0, OPT_CUSTOM_POINTS },\
	{ "set-custom-step",           required_argument, 0, OPT_CUSTOM_STEP },\
	{ "set-custom-type",           required_argument, 0, OPT_CUSTOM_TYPE },\
	{ "set-rotation-angle",        required_argument, 0, OPT_ROTATION_ANGLE }, \
	{ "set-pressure-range",        required_argument, 0, OPT_PRESSURE_RANGE }, \
	{ "set-calibration",           required_argument, 0, OPT_CALIBRATION }, \
	{ "set-area",                  required_argument, 0, OPT_AREA }, \
	{ "set-eraser-button-mode",    required_argument, 0, OPT_ERASER_BUTTON_MODE }, \
	{ "set-eraser-button-button",  required_argument, 0, OPT_ERASER_BUTTON_BUTTON },\
	{ "set-plugin-path",	       required_argument, 0, OPT_PLUGIN_PATH }

/* Note: New arguments should be added to shell completions */

static inline void
tools_print_usage_option_list(struct option *opts)
{
	printf("Options:\n");

	struct option *o = opts;
	while (o && o->name) {
		if (strstartswith(o->name, "enable-") &&
		    strstartswith((o + 1)->name, "disable-")) {
			printf("   --%s/--%s\n", o->name, (o + 1)->name);
			o++;
		} else {
			printf("   --%s\n", o->name);
		}
		o++;
	}
}

enum tools_backend { BACKEND_NONE, BACKEND_DEVICE, BACKEND_UDEV };

struct tools_options {
	char match[256];

	int plugins;
	char **plugin_paths;

	int tapping;
	int drag;
	int drag_lock;
	int natural_scroll;
	int left_handed;
	int middlebutton;
	enum libinput_config_click_method click_method;
	enum libinput_config_clickfinger_button_map clickfinger_map;
	enum libinput_config_scroll_method scroll_method;
	enum libinput_config_tap_button_map tap_map;
	int scroll_button;
	int scroll_button_lock;
	double speed;
	int dwt;
	int dwtp;
	enum libinput_config_accel_profile profile;
	char disable_pattern[64];
	enum libinput_config_accel_type custom_type;
	double custom_step;
	size_t custom_npoints;
	double *custom_points;
	unsigned int angle;
	double pressure_range[2];
	float calibration[6];
	struct libinput_config_area_rectangle area;
	enum libinput_config_3fg_drag_state drag_3fg;
	enum libinput_config_send_events_mode sendevents;
	enum libinput_config_eraser_button_mode eraser_button_mode;
	unsigned int eraser_button_button;
};

void
tools_init_options(struct tools_options *options);
int
tools_parse_option(int option, const char *optarg, struct tools_options *options);
struct libinput *
tools_open_backend(enum tools_backend which,
		   const char **seat_or_devices,
		   bool verbose,
		   bool *grab,
		   bool with_plugins,
		   char **plugin_paths);
void
tools_device_apply_config(struct libinput_device *device,
			  struct tools_options *options);
void
tools_tablet_tool_apply_config(struct libinput_tablet_tool *tool,
			       struct tools_options *options);
int
tools_exec_command(const char *prefix, int argc, char **argv);

bool
find_touchpad_device(char *path, size_t path_len);
bool
is_touchpad_device(const char *devnode);

void
tools_list_device_quirks(struct quirks_context *ctx,
			 struct udev_device *device,
			 void (*callback)(void *userdata, const char *str),
			 void *userdata);

void
tools_dispatch(struct libinput *libinput);
#endif
