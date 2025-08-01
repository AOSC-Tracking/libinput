/*
 * Copyright © 2018 Red Hat, Inc.
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

#include "builddir.h"
#include "libinput-util.h"

int
main(int argc, char **argv)
{
	char *builddir = NULL;
	char *mode;

	assert(argc == 2);
	mode = argv[1];

	bool is_builddir = builddir_lookup(&builddir);
	if (streq(mode, "--builddir-is-null")) {
		assert(!is_builddir);
		assert(builddir == NULL);
	} else if (streq(mode, "--builddir-is-set")) {
		/* In the case of release builds, the builddir is
		   the empty string */
		if (streq(MESON_BUILD_ROOT, "")) {
			assert(!is_builddir);
			assert(builddir == NULL);
		} else {
			assert(is_builddir);
			assert(builddir);
			assert(streq(MESON_BUILD_ROOT, builddir));
		}
	} else {
		abort();
	}

	free(builddir);

	return 0;
}
