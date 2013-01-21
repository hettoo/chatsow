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

static char *server;
static char *level;
static char *game;
static char *host;
static char *port;

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

static bool stopped = FALSE;

void ui_stop() {
    endwin();
    stopped = TRUE;
}

static void interrupt(int sig) {
    quit();
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

static int draw_colored(WINDOW *win, char *string, qboolean ignore) {
    int len = 0;
    qboolean set_color = qfalse;
    int i;
    for (i = 0; string[i]; i++) {
        if (set_color) {
            set_color = qfalse;
            if (string[i] >= '0' && string[i] <= '9') {
                int color = string[i] - '0';
                if (color == 0)
                    color = 10;
                if (!ignore)
                    wattrset(win, COLOR_PAIR(color));
                continue;
            } else if (string[i] != '^') {
                waddch(win, '^');
            }
        } else if (string[i] == '^') {
            set_color = qtrue;
            continue;
        }
        len++;
        waddch(win, string[i]);
    }
    if (set_color) {
        waddch(win, '^');
        len++;
    }
    return len;
}

static void draw_titlewin() {
    werase(titlewin);
    int i = 1;
    waddstr(titlewin, " ");
    if (server) {
        wattron(titlewin, A_BOLD);
        i += draw_colored(titlewin, server, qtrue);
        wattroff(titlewin, A_BOLD);
        i += draw_colored(titlewin, " ", qtrue);
    }
    if (level) {
        i += draw_colored(titlewin, "[", qtrue);
        wattron(titlewin, A_BOLD);
        i += draw_colored(titlewin, level, qtrue);
        wattroff(titlewin, A_BOLD);
        i += draw_colored(titlewin, "] ", qtrue);
    }
    if (game) {
        i += draw_colored(titlewin, "[", qtrue);
        wattron(titlewin, A_BOLD);
        i += draw_colored(titlewin, game, qtrue);
        wattroff(titlewin, A_BOLD);
        i += draw_colored(titlewin, "] ", qtrue);
    }
    if (host || port)
        i += draw_colored(titlewin, "@ ", qtrue);
    if (host) {
        wattron(titlewin, A_BOLD);
        i += draw_colored(titlewin, host, qtrue);
        wattroff(titlewin, A_BOLD);
    }
    if (port) {
        i += draw_colored(titlewin, ":", qtrue);
        i += draw_colored(titlewin, port, qtrue);
    }
    for (; i < COLS; i++)
        waddch(titlewin, ' ');
    wrefresh(titlewin);
}

void set_title(char *new_server, char *new_level, char *new_game, char *new_host, char *new_port) {
    server = new_server;
    level = new_level;
    game = new_game;
    host = new_host;
    port = new_port;
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
        draw_colored(outwin, buffer[index], qfalse);
    }
    wrefresh(outwin);
}

void draw_status(char *name) {
    werase(statuswin);
    static char string[32];
    int i = 0;
    i += draw_colored(statuswin, " [", qtrue);
    timestring(string);
    wattron(statuswin, A_BOLD);
    i += draw_colored(statuswin, string, qtrue);
    wattroff(statuswin, A_BOLD);
    i += draw_colored(statuswin, "] ", qtrue);
    if (name != NULL) {
        wattron(statuswin, A_BOLD);
        i += draw_colored(statuswin, name, qtrue);
        wattroff(statuswin, A_BOLD);
    }
    for (; i < COLS; i++)
        waddch(statuswin, ' ');
    wrefresh(statuswin);
}

static void draw_inwin() {
    werase(inwin);
    wattrset(inwin, COLOR_PAIR(7));
    wattron(inwin, A_BOLD);
    draw_colored(inwin, "> ", qfalse);
    wattroff(inwin, A_BOLD);
    if (commandline[0] != '/')
        wattrset(inwin, COLOR_PAIR(2));
    commandline[commandline_length] = '\0';
    draw_colored(inwin, commandline, qfalse);
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
            ghost_line = !empty || !allow_time;
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
    signal(SIGINT, interrupt);
    signal(SIGSEGV, interrupt);

    mainwin = initscr();
    init_colors();
    nonl();
    cbreak();
    noecho();
    curs_set(0);

    titlewin = subwin(mainwin, 1, COLS, 0, 0);
    wattrset(titlewin, COLOR_PAIR(11));

    outwin = subwin(mainwin, LINES - 3, COLS, 1, 0);
    scrollok(outwin, TRUE);

    statuswin = subwin(mainwin, 1, COLS, LINES - 2, 0);
    wattrset(statuswin, COLOR_PAIR(11));

    inwin = subwin(mainwin, 1, COLS, LINES - 1, 0);
    keypad(inwin, TRUE);

    set_title(NULL, NULL, NULL, NULL, NULL);
    draw_outwin();
    draw_status(NULL);
    draw_inwin();
}

void ui_run() {
    wtimeout(inwin, INPUT_TIME);
    for (;;) {
        if (stopped)
            break;
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
                        client_command("say %s", commandline);
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
