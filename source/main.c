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

#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

int die(char *message) {
    printf("%s", message);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        die("No demo given\n");
    }
    FILE *fp = fopen(argv[1], "r");
    parse_demo(fp);
    fclose(fp);
    return 0;
}
