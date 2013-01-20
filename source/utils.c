/*
Copyright (C) 2013 hettoo (Gerco van Heerdt)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "main.h"

int die(char *format, ...) {
    shutdown();
	va_list	argptr;
	va_start(argptr, format);
    vprintf(format, argptr);
	va_end(argptr);
    printf("\n");
    exit(1);
}

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

unsigned int millis() {
	struct timeval tp;
	struct timezone tzp;

	gettimeofday( &tp, &tzp );

	return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

int timestring(char *string) {
    time_t raw_time;
    time(&raw_time);
    strftime(string, 6, "%H:%M", localtime(&raw_time));
    return strlen(string);
}
