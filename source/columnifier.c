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

#include "utils.h"
#include "columnifier.h"

void columnifier_init(columnifier_t *c, int max_visual_width, int max_width) {
    c->max_visual_width = max_visual_width;
    c->max_width = max_width;
    c->actual_max_width = -1;
    c->actual_max_visual_width = -1;
    c->index = 0;
    c->real_index = 0;
}

void columnifier_preprocess(columnifier_t *c, char *s) {
    int len = uncolored_length(s) + 2;
    if (c->actual_max_visual_width == -1 || len > c->actual_max_visual_width)
        c->actual_max_visual_width = len;
    len = strlen(s) + 6;
    if (c->actual_max_width == -1 || len > c->actual_max_width)
        c->actual_max_width = len;
}

void columnifier_process(columnifier_t *c, char *r, char *s) {
    int columns = min(c->max_visual_width / c->actual_max_visual_width, c->max_width / c->actual_max_width);
    if (c->index == 0 && c->real_index != 0)
        *(r++) = '\n';
    strcpy(r, s);
    int len = strlen(r);
    int actual_len = uncolored_length(r);
    r[len++] = '^';
    r[len++] = '7';
    for (; actual_len < c->actual_max_visual_width; actual_len++)
        r[len++] = ' ';
    r[len] = '\0';
    c->index = (c->index + 1) % columns;
    c->real_index++;
}

void columnifier_finish(columnifier_t *c, char *r) {
    int i = 0;
    if (c->real_index != 0)
        r[i++] = '\n';
    r[i] = '\0';
}
