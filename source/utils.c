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

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "utils.h"

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

void parse(char *string, void (*f_char)(char c), void (*f_ghost)(char c), void (*f_color)(int color)) {
    parse_state_t state;
    parse_init(&state, f_char, f_ghost, f_color, '\0');
    parse_interleaved(string, &state);
    parse_finish(&state);
}

void parse_init(parse_state_t *state, void (*f_char)(char c), void (*f_ghost)(char c), void (*f_color)(int color), char separator) {
    state->set_color = qfalse;
    state->f_char = f_char;
    state->f_ghost = f_ghost;
    state->f_color = f_color;
    state->separator = separator;
}

char *parse_interleaved(char *string, parse_state_t *state) {
    int i;
    for (i = 0; string[i] != state->separator && string[i]; i++) {
        if (state->set_color) {
            state->set_color = qfalse;
            if (string[i] >= '0' && string[i] <= '9') {
                int color = string[i] - '0';
                if (state->f_color)
                    state->f_color(color);
                continue;
            } else {
                if (string[i] == '^') {
                    if (state->f_ghost)
                        state->f_ghost('^');
                } else {
                    if (state->f_char)
                        state->f_char('^');
                }
            }
        } else if (string[i] == '^') {
            state->set_color = qtrue;
            continue;
        }
        if (state->f_char)
            state->f_char(string[i]);
    }
    return string + i + 1;
}

char *parse_peek(char *string, parse_state_t *state) {
    parse_state_t backup = *state;
    state->f_char = NULL;
    state->f_color = NULL;
    char *result = parse_interleaved(string, state);
    *state = backup;
    return result;
}

static qboolean parse_empty_result;

static void f_empty_char(char c) {
    if (c == '\n' || c != ' ')
        parse_empty_result = qfalse;
}

qboolean parse_empty_last(char *string) {
    parse_empty_result = qtrue;
    parse_state_t state;
    parse_init(&state, f_empty_char, NULL, NULL, '\0');
    parse_interleaved(string, &state);
    parse_finish(&state);
    return parse_empty_result;
}

void parse_finish(parse_state_t *state) {
    if (state->set_color && state->f_char)
        state->f_char('^');
}

static char uncolor_result[2048];
static int uncolor_length;

static void uncolor_char(char c) {
    uncolor_result[uncolor_length++] = c;
}

char *uncolor(char *string) {
    uncolor_length = 0;
    parse(string, uncolor_char, NULL, NULL);
    uncolor_result[uncolor_length] = '\0';
    return uncolor_result;
}

int uncolored_length(char *string) {
    uncolor_length = 0;
    parse(string, uncolor_char, NULL, NULL);
    return uncolor_length;
}

static qboolean real;
static int other_index;
static int real_length;
static int reindex_result;

void check_reindex_result() {
    if (real) {
        if (uncolor_length == other_index)
            reindex_result = real_length;
    } else {
        if (real_length == other_index)
            reindex_result = uncolor_length;
    }
}

static void reindex_char(char c) {
    real_length++;
    uncolor_char(c);
    check_reindex_result();
}

static void reindex_ghost(char c) {
    real_length++;
    check_reindex_result();
}

static void reindex_color(int color) {
    real_length += 2;
    check_reindex_result();
}

static int reindex(char *string, int index) {
    other_index = index;
    uncolor_length = 0;
    real_length = 0;
    reindex_result = -1;
    check_reindex_result();
    parse(string, reindex_char, reindex_ghost, reindex_color);
    return reindex_result;
}

int real_index(char *string, int uncolored_index) {
    real = qtrue;
    return reindex(string, uncolored_index);
}

int uncolored_index(char *string, int real_index) {
    real = qfalse;
    return reindex(string, real_index);
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

qboolean starts_with(const void *a_raw, const void *b_raw) {
    char *a = (char *)a_raw;
    char *b = (char *)b_raw;
    strcpy(uncolored_a, uncolor(a));
    strcpy(uncolored_b, uncolor(b));
    while (*b) {
        if (!*a || *a != *b)
            return qfalse;
        a++;
        b++;
    }
    return qtrue;
}

qboolean rm(void *array, int element_size, int *size, qboolean (*f_test)(void *x)) {
    int i;
    int skip = 0;
    int total = *size * element_size;
    for (i = 0; i < total; i += element_size) {
        if (skip > 0)
            memcpy(array + i - skip, array + i, element_size);
        if (f_test(array + i))
            skip += element_size;
    }
    *size -= skip / element_size;
    return skip > 0;
}
