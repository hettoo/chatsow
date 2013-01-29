/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#ifndef WRLC_CS_H
#define WRLC_CS_H

#include "import.h"

typedef struct cs_s {
    char css[MAX_CONFIGSTRINGS][MAX_CONFIGSTRING_CHARS];
} cs_t;

void cs_init(cs_t *cs);
void cs_set(cs_t *cs, int index, char *string);
char *cs_get(cs_t *cs, int index);
char *player_name(cs_t *cs, int num);

#endif
