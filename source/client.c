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


#include <pthread.h>

#include "import.h"
#include "ui.h"

static qboolean started = qfalse;
static pthread_t thread;

static void *client_run(void *args) {
    return NULL;
}

void client_start() {
    pthread_create(&thread, NULL, client_run, NULL);
    started = qtrue;
}

void client_stop() {
    if (started)
        pthread_join(thread, NULL);
}

void execute(char *command) {
    ui_output(command);
}
