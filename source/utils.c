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
#include "utils.h"

int die(char *format, ...) {
    quit();
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

void parse(char *string, void (*f_char)(char c), void (*f_color)(int color)) {
    qboolean set_color = qfalse;
    int i;
    for (i = 0; string[i]; i++) {
        if (set_color) {
            set_color = qfalse;
            if (string[i] >= '0' && string[i] <= '9') {
                int color = string[i] - '0';
                if (color == 0)
                    color = 10;
                if (f_color)
                    f_color(color);
                continue;
            } else if (string[i] != '^' && f_char) {
                f_char('^');
            }
        } else if (string[i] == '^') {
            set_color = qtrue;
            continue;
        }
        if (f_char)
            f_char(string[i]);
    }
    if (set_color && f_char) {
        f_char('^');
    }
}

static char uncolor_result[2048];
static int uncolor_length;

static void uncolor_char(char c) {
    uncolor_result[uncolor_length++] = c;
}

char *uncolor(char *string) {
    uncolor_length = 0;
    parse(string, uncolor_char, NULL);
    uncolor_result[uncolor_length] = '\0';
    return uncolor_result;
}

static char uncolored_a[MAX_STRING_CHARS];
static char uncolored_b[MAX_STRING_CHARS];

qboolean partial_match(char *a, char *b) {
    strcpy(uncolored_a, uncolor(a));
    int len_a = strlen(uncolored_a);
    if (len_a == 0)
        return qtrue;
    strcpy(uncolored_b, uncolor(b));
    int len_b = strlen(uncolored_b);
    int i;
    for (i = 0; i < len_b - len_a + 1; i++) {
        int result = strncasecmp(uncolored_a, uncolored_b + i, len_a);
        if (result == 0)
            return qtrue;
    }
    return qfalse;
}

int insensitive_cmp(const void *a_raw, const void *b_raw) {
    char *a = (char *)a_raw;
    char *b = (char *)b_raw;
    strcpy(uncolored_a, uncolor(a));
    strcpy(uncolored_b, uncolor(b));
    return strcasecmp(uncolored_a, uncolored_b);
}
