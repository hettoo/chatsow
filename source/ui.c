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
#include <string.h>
#include <curses.h>
#include <signal.h>
#include <pthread.h>

#include "main.h"
#include "utils.h"
#include "client.h"

static pthread_mutex_t mutex;

static WINDOW *mainwin;
static WINDOW *outwin;
static WINDOW *statuswin;
static WINDOW *inwin;

static char buffer[512][128];
static int buffer_count = 0;

static char commandline[512];
static int commandline_length = 0;

void ui_stop() {
    pthread_mutex_destroy(&mutex);
    endwin();
}

static void interrupt(int sig) {
    shutdown();
    exit(sig);
}

static void init_colors() {
    if (has_colors()) {
        start_color();

        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);
        init_pair(8, COLOR_YELLOW, COLOR_BLACK);
        init_pair(9, COLOR_WHITE, COLOR_BLACK);
        init_pair(10, COLOR_BLACK, COLOR_BLACK);
        init_pair(11, COLOR_BLACK, COLOR_WHITE);
    }
}

static void draw_outwin(bool refresh) {
    werase(outwin);
    int outheight = LINES - 2;
    int i;
    pthread_mutex_lock(&mutex);
    int start = max(0, buffer_count - outheight);
    int displayable = buffer_count - start;
    for (i = start; i < buffer_count; i++) {
        wmove(outwin, i - start + outheight - displayable, 0);
        waddstr(outwin, buffer[i]);
    }
    pthread_mutex_unlock(&mutex);
    if (refresh)
        wrefresh(outwin);
}

static void draw_statuswin(bool refresh) {
    werase(statuswin);
    int i = 0;
    wattron(statuswin, A_BOLD);
    for (; i < COLS; i++)
        waddch(statuswin, ' ');
    wattroff(statuswin, A_BOLD);
    if (refresh)
        wrefresh(statuswin);
}

static void draw_inwin(bool refresh) {
    werase(inwin);
    wattron(inwin, A_BOLD);
    waddstr(inwin, "> ");
    wattroff(inwin, A_BOLD);
    int i;
    for (i = 0; i < commandline_length; i++)
        waddch(inwin, commandline[i]);
    if (refresh)
        wrefresh(inwin);
}

void ui_output(char *format, ...) {
	va_list	argptr;
	va_start(argptr, format);
    pthread_mutex_lock(&mutex);
    vsprintf(buffer[buffer_count++], format, argptr);
    pthread_mutex_unlock(&mutex);
	va_end(argptr);
    draw_outwin(TRUE);
}

void ui_init() {
    pthread_mutex_init(&mutex, NULL);
}

void ui_run() {
    signal(SIGINT, interrupt);
    signal(SIGSEGV, interrupt);

    mainwin = initscr();
    nonl();
    cbreak();
    noecho();

    init_colors();

    outwin = subwin(mainwin, LINES - 2, COLS, 0, 0);
    scrollok(outwin, TRUE);
    leaveok(outwin, TRUE);

    statuswin = subwin(mainwin, 1, COLS, LINES - 2, 0);
    wattrset(statuswin, COLOR_PAIR(11));

    inwin = subwin(mainwin, 1, COLS, LINES - 1, 0);
    keypad(inwin, TRUE);

    draw_outwin(TRUE);
    draw_statuswin(TRUE);
    draw_inwin(TRUE);

    for (;;) {
        int c = wgetch(inwin);
        switch (c) {
            case KEY_BACKSPACE:
            case 127:
                if (commandline_length > 0)
                    commandline_length--;
                break;
            case KEY_ENTER:
            case 13:
                commandline[commandline_length] = '\0';
                execute(commandline);
                commandline_length = 0;
                draw_outwin(TRUE);
                break;
            default:
                commandline[commandline_length++] = c;
                break;
        }
        draw_inwin(TRUE);
    }
}
