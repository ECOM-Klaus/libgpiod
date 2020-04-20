// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2020 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <poll.h>
#include <string.h>

#include "gpiod-test.h"

#define GPIOD_TEST_GROUP "watch"

/*
 * TODO We could probably improve the testing framework with support for a
 * thread pool both for regular line events and for line state changes here.
 * For now we'll just trigger the latter synchronously.
 */

GPIOD_TEST_CASE(single_request_event, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_watch_event event;
	struct timespec ts = { 1, 0 };
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line_watched(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 1);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_read(chip, &event);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(event.event_type, ==, GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(line), ==,
			gpiod_line_offset(event.line));
}

GPIOD_TEST_CASE(poll_watch_fd, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_watch_event event;
	struct gpiod_line *line;
	struct pollfd pfd;
	gint ret, fd;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line_watched(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	fd = gpiod_chip_watch_get_fd(chip);
	g_assert_cmpint(fd, >=, 0);
	gpiod_test_return_if_failed();

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	pfd.events = POLLIN | POLLPRI;

	ret = poll(&pfd, 1, 10);
	/*
	 * We're expecting timeout - there must not be any events in the
	 * kernel queue.
	 */
	g_assert_cmpint(ret, ==, 0);

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = poll(&pfd, 1, 1000);
	g_assert_cmpint(ret, >, 0);

	ret = gpiod_chip_watch_event_read(chip, &event);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(event.event_type, ==, GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(line), ==,
			gpiod_line_offset(event.line));
}

GPIOD_TEST_CASE(getting_fd_when_with_lines_watched, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_get_fd(chip);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EPERM);
}

GPIOD_TEST_CASE(try_to_watch_line_twice, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line_watched(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_watch(line);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EBUSY);
}

GPIOD_TEST_CASE(try_to_unwatch_non_watched_line, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_unwatch(line);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EBUSY);
}
