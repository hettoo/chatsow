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

#define MAX_BUFFER_SIZE 512
#define MAX_OUTPUT_LENGTH 128
#define MAX_INPUT_LENGTH 128

static pthread_mutex_t mutex;

static WINDOW *mainwin;
static WINDOW *titlewin;
static WINDOW *outwin;
static WINDOW *statuswin;
static WINDOW *inwin;

static char buffer[MAX_BUFFER_SIZE][MAX_OUTPUT_LENGTH];
static int buffer_count = 0;
static int buffer_index = 0;
static int output_length = 0;

static char commandline[MAX_INPUT_LENGTH];
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
        init_pair(11, COLOR_BLACK, COLOR_BLUE);
    }
}

static void draw_titlewin(bool refresh) {
    werase(titlewin);
    int i = 0;
    wattron(titlewin, A_BOLD);
    for (; i < COLS; i++)
        waddch(titlewin, ' ');
    wattroff(titlewin, A_BOLD);
    if (refresh)
        wrefresh(titlewin);
}

static void draw_outwin(bool refresh) {
    werase(outwin);
    int outheight = LINES - 3;
    int i;
    pthread_mutex_lock(&mutex);
    int start = max(0, buffer_count - outheight);
    int displayable = buffer_count - start;
    for (i = start; i < buffer_count; i++) {
        wmove(outwin, i - start + outheight - displayable, 0);
        waddstr(outwin, buffer[(i - buffer_count // this is incorrect, but apparantly there is another error correcting this :D
                    + buffer_index + MAX_BUFFER_SIZE) % MAX_BUFFER_SIZE]);
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
    static char string[65536];
	va_list	argptr;
	va_start(argptr, format);
    vsprintf(string, format, argptr);
	va_end(argptr);
    int len = strlen(string);
    int i;
    int maxlen = min(COLS, MAX_OUTPUT_LENGTH - 2);
    pthread_mutex_lock(&mutex);
    if (buffer_count == 0)
        buffer_count++;
    for (i = 0; i < len; i++) {
        if (string[i] == '\n' || output_length > maxlen) {
            buffer[buffer_index][output_length] = '\0';
            if (buffer_count < MAX_BUFFER_SIZE)
                buffer_count++;
            buffer_index = (buffer_index + 1) % MAX_BUFFER_SIZE;
            output_length = 0;
        }
        if (string[i] != '\n')
            buffer[buffer_index][output_length++] = string[i];
    }
    pthread_mutex_unlock(&mutex);
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
    curs_set(0);

    init_colors();

    titlewin = subwin(mainwin, 1, COLS, 0, 0);
    wattrset(titlewin, COLOR_PAIR(11));

    outwin = subwin(mainwin, LINES - 3, COLS, 1, 0);
    scrollok(outwin, TRUE);

    statuswin = subwin(mainwin, 1, COLS, LINES - 2, 0);
    wattrset(statuswin, COLOR_PAIR(11));

    inwin = subwin(mainwin, 1, COLS, LINES - 1, 0);
    keypad(inwin, TRUE);

    draw_titlewin(TRUE);
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
