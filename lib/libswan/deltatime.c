/* time objects and functions, for libreswan
 *
 * Copyright (C) 1998, 1999, 2000  Henry Spencer.
 * Copyright (C) 1999, 2000, 2001  Richard Guy Briggs
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/lgpl.txt>.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 * License for more details.
 *
 */

#include <stdbool.h>

#include "constants.h"
#include "deltatime.h"

/* delta time (interval) operations */
deltatime_t deltatime(time_t secs)
{
	return (deltatime_t) DELTATIME(secs);
}

/* delta time (interval) operations */
deltatime_t deltatime_ms(intmax_t ms)
{
	return (deltatime_t) DELTATIME_MS(ms);
}

int deltatime_cmp(deltatime_t a, deltatime_t b)
{
	/* return sign(a - b) */
	if (timercmp(&a.dt, &b.dt, <)) {
		return -1;
	} else if (timercmp(&a.dt, &b.dt, >)) {
		return +1;
	} else {
		return 0;
	}
}

deltatime_t deltatime_max(deltatime_t a, deltatime_t b)
{
	if (deltatime_cmp(a, b) > 0) {
		return a;
	} else {
		return b;
	}
}

intmax_t deltamillisecs(deltatime_t d)
{
	return d.dt.tv_sec * 1000 + d.dt.tv_usec / 1000;
}

time_t deltasecs(deltatime_t d)
{
	return d.dt.tv_sec;
}

deltatime_t deltatimescale(int num, int denom, deltatime_t d)
{
	/* ??? should check for overflow */
	return deltatime(deltasecs(d) * num / denom);
}

bool deltaless(deltatime_t a, deltatime_t b)
{
	return deltatime_cmp(a, b) < 0;
}

bool deltaless_tv_dt(const struct timeval a, const deltatime_t b)
{
	return a.tv_sec < deltasecs(b);
}