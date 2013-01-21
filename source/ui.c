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
#include <time.h>

#include "main.h"
#include "utils.h"
#include "client.h"
#include "cmd.h"
#include "ui.h"

#define MAX_FULLLENGTH 512
#define MAX_BUFFER_SIZE 512
#define MAX_OUTPUT_LENGTH 128
#define MAX_INPUT_LENGTH 128

#define SCROLL_SPEED 10

#define INPUT_TIME 10

static WINDOW *mainwin;
static WINDOW *titlewin;
static WINDOW *outwin;
static WINDOW *statuswin;
static WINDOW *inwin;

static char title[MAX_FULLLENGTH];

static char buffer[MAX_BUFFER_SIZE][MAX_OUTPUT_LENGTH];
static int buffer_count = 0;
static qboolean ghost_line = qfalse;
static int buffer_index = 0;
static int output_length = 0;
static int scroll_up = 0;
static qboolean next_line = qfalse;
static qboolean allow_time = qtrue;

static char commandline[MAX_INPUT_LENGTH];
static int commandline_length = 0;

void ui_stop() {
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
        init_pair(4, COLOR_WHITE, COLOR_BLUE);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);
        init_pair(8, COLOR_YELLOW, COLOR_BLACK);
        init_pair(9, COLOR_WHITE, COLOR_BLACK);
        init_pair(10, COLOR_BLACK, COLOR_WHITE);
        init_pair(11, COLOR_WHITE, COLOR_BLUE);
    }
}

static void draw_titlewin() {
    werase(titlewin);
    int i;
    wattron(titlewin, A_BOLD);
    waddch(titlewin, ' ');
    qboolean end = qfalse;
    for (i = 0; i < COLS - 1; i++) {
        if (!title[i])
            end = qtrue;
        if (end)
            waddch(titlewin, ' ');
        else
            waddch(titlewin, title[i]);
    }
    wattroff(titlewin, A_BOLD);
    wrefresh(titlewin);
}

void set_title(char *format, ...) {
	va_list	argptr;
	va_start(argptr, format);
    vsprintf(title, format, argptr);
	va_end(argptr);
    draw_titlewin();
}

static void draw_outwin() {
    werase(outwin);
    int outheight = LINES - 3;
    int i;
    wattrset(outwin, COLOR_PAIR(7));
    int actual_buffer_count = buffer_count - (ghost_line ? 1 : 0);
    if (scroll_up > actual_buffer_count - outheight)
        scroll_up = actual_buffer_count - outheight;
    if (scroll_up < 0)
        scroll_up = 0;
    int start = max(0, actual_buffer_count - outheight);
    int displayable = actual_buffer_count - start;
    for (i = start; i < actual_buffer_count; i++) {
        wmove(outwin, i - start + outheight - displayable, 0);
        int index = i;
        index -= buffer_count - 1; // replace the last buffer with
        index += buffer_index; // the selected one
        index -= scroll_up;
        index += MAX_BUFFER_SIZE * 2; // negative modulo prevention
        index %= MAX_BUFFER_SIZE;
        int j;
        qboolean set_color = qfalse;
        for (j = 0; buffer[index][j]; j++) {
            if (set_color) {
                set_color = qfalse;
                if (buffer[index][j] >= '0' && buffer[index][j] <= '9') {
                    int color = buffer[index][j] - '0';
                    if (color == 0)
                        color = 10;
                    wattrset(outwin, COLOR_PAIR(color));
                    continue;
                } else if (buffer[index][j] != '^') {
                    waddch(outwin, '^');
                }
            } else if (buffer[index][j] == '^') {
                set_color = qtrue;
                continue;
            }
            waddch(outwin, buffer[index][j]);
        }
    }
    wrefresh(outwin);
}

void draw_status(char *name) {
    werase(statuswin);
    static char string[32];
    int i = 2;
    waddstr(statuswin, " [");
    i += timestring(string);
    wattron(statuswin, A_BOLD);
    waddstr(statuswin, string);
    wattroff(statuswin, A_BOLD);
    waddstr(statuswin, "] ");
    i += 2;
    wattron(statuswin, A_BOLD);
    if (name != NULL) {
        waddstr(statuswin, name);
        i += strlen(name);
    }
    wattroff(statuswin, A_BOLD);
    for (; i < COLS; i++)
        waddch(statuswin, ' ');
    wrefresh(statuswin);
}

static void draw_inwin() {
    werase(inwin);
    wattrset(inwin, COLOR_PAIR(7));
    wattron(inwin, A_BOLD);
    waddstr(inwin, "> ");
    wattroff(inwin, A_BOLD);
    if (commandline[0] != '/')
        wattrset(inwin, COLOR_PAIR(2));
    int i;
    for (i = 0; i < commandline_length; i++)
        waddch(inwin, commandline[i]);
    waddch(inwin, '_');
    wrefresh(inwin);
}

void ui_output_real(char *string) {
    int len = strlen(string);
    int i;
    int maxlen = min(COLS, MAX_OUTPUT_LENGTH - 2);
    qboolean empty = qfalse;
    if (ghost_line)
        ghost_line = qfalse;
    if (buffer_count == 0)
        buffer_count++;
    for (i = 0; i < len; i++) {
        if (next_line) {
            if (buffer_count < MAX_BUFFER_SIZE)
                buffer_count++;
            buffer_index = (buffer_index + 1) % MAX_BUFFER_SIZE;
            if (scroll_up)
                scroll_up++;
            output_length = 0;
            next_line = qfalse;
        }
        if (string[i] == '\n' || output_length > maxlen) {
            buffer[buffer_index][output_length] = '\0';
            next_line = qtrue;
            empty = qfalse;
            if (len > i + 1) {
                empty = qtrue;
                int j;
                for (j = i + 1; j < len && string[j] != '\n'; j++) {
                    if ((j - (i + 1)) % 2 == 0) {
                        if (string[j] != '^')
                            empty = qfalse;
                    } else {
                        if (string[j] < '0' || string[j] > '9')
                            empty = qfalse;
                    }
                }
                if ((j - (i + 1)) % 2 == 1)
                    empty = qfalse;
            }
            allow_time = string[i] == '\n';
            ghost_line = !empty || !allow_time;//empty && !allow_time;
        } else {
            if (ghost_line && output_length == 0) {
                int j;
                for (j = 0; j < min(6, COLS - 2); j++)
                    buffer[buffer_index][output_length++] = ' ';
                allow_time = qfalse;
            }
            allow_time = empty;
            buffer[buffer_index][output_length++] = string[i];
        }
    }
    ghost_line = empty;
}

void ui_output(char *format, ...) {
    static char string[65536];
    string[0] = '^';
    string[1] = '7';
    int len = 2;
    if (allow_time) {
        len += timestring(string + len);
        string[len++] = ' ';
    }

	va_list	argptr;
	va_start(argptr, format);
    vsprintf(string + len, format, argptr);
	va_end(argptr);
    ui_output_real(string);
    draw_outwin();
}

void ui_init() {
    title[0] = '\0';

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

    draw_titlewin();
    draw_outwin();
    draw_status(NULL);
    draw_inwin();
}

void ui_run() {
    wtimeout(inwin, INPUT_TIME);
    for (;;) {
        client_frame();
        int c = wgetch(inwin);
        if (c == -1)
            continue;
        bool handled = TRUE;
        switch (c) {
            case KEY_PPAGE:
                scroll_up += SCROLL_SPEED;
                break;
            case KEY_NPAGE:
                scroll_up -= SCROLL_SPEED;
                break;
            default:
                handled = FALSE;
                break;
        }
        if (handled) {
            draw_outwin();
            continue;
        }
        switch (c) {
            case KEY_BACKSPACE:
            case 127:
                if (commandline_length > 0)
                    commandline_length--;
                break;
            case KEY_ENTER:
            case 13:
                commandline[commandline_length] = '\0';
                scroll_up = 0;
                if (commandline_length > 0) {
                    if (commandline[0] == '/') {
                        ui_output("%s\n", commandline);
                        cmd_execute(commandline + 1);
                    } else {
                        if (client_ready())
                            client_command("say %s", commandline);
                        else
                            ui_output("not connected\n");
                    }
                    commandline_length = 0;
                }
                draw_outwin();
                break;
            default:
                commandline[commandline_length++] = c;
                break;
        }
        draw_inwin();
    }
}
