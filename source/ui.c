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

typedef struct screen_s {
    char *server;
    char *level;
    char *game;
    char *host;
    char *port;

    char *last_name;

    char buffer[MAX_BUFFER_SIZE][MAX_OUTPUT_LENGTH];
    int buffer_count;
    qboolean ghost_line;
    int buffer_index;
    int output_length;
    int scroll_up;
    qboolean next_line;
    qboolean allow_time;

    char commandline[MAX_INPUT_LENGTH];
    int commandline_length;

    qboolean updated;
    qboolean important;
} screen_t;

static screen_t screens[SCREENS];
static int screen = 0;

static bool stopped = FALSE;

void ui_stop() {
    int i;
    for (i = 0; i < CLIENT_SCREENS; i++)
        disconnect(i);
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
    if (screens[screen].server) {
        wattron(titlewin, A_BOLD);
        i += draw_colored(titlewin, screens[screen].server, qtrue);
        wattroff(titlewin, A_BOLD);
        i += draw_colored(titlewin, " ", qtrue);
    }
    if (screens[screen].level) {
        i += draw_colored(titlewin, "[", qtrue);
        wattron(titlewin, A_BOLD);
        i += draw_colored(titlewin, screens[screen].level, qtrue);
        wattroff(titlewin, A_BOLD);
        i += draw_colored(titlewin, "] ", qtrue);
    }
    if (screens[screen].game) {
        i += draw_colored(titlewin, "[", qtrue);
        wattron(titlewin, A_BOLD);
        i += draw_colored(titlewin, screens[screen].game, qtrue);
        wattroff(titlewin, A_BOLD);
        i += draw_colored(titlewin, "] ", qtrue);
    }
    if (screens[screen].host || screens[screen].port)
        i += draw_colored(titlewin, "@ ", qtrue);
    if (screens[screen].host) {
        wattron(titlewin, A_BOLD);
        i += draw_colored(titlewin, screens[screen].host, qtrue);
        wattroff(titlewin, A_BOLD);
    }
    if (screens[screen].port) {
        i += draw_colored(titlewin, ":", qtrue);
        i += draw_colored(titlewin, screens[screen].port, qtrue);
    }
    for (; i < COLS; i++)
        waddch(titlewin, ' ');
    wrefresh(titlewin);
}

void set_title(int client, char *new_server, char *new_level, char *new_game, char *new_host, char *new_port) {
    screens[client + 1].server = new_server;
    screens[client + 1].level = new_level;
    screens[client + 1].game = new_game;
    screens[client + 1].host = new_host;
    screens[client + 1].port = new_port;
    draw_titlewin(client + 1);
}

static void draw_outwin() {
    werase(outwin);
    int outheight = LINES - 3;
    int i;
    wattrset(outwin, COLOR_PAIR(7));
    int actual_buffer_count = screens[screen].buffer_count - (screens[screen].ghost_line ? 1 : 0);
    if (screens[screen].scroll_up > actual_buffer_count - outheight)
        screens[screen].scroll_up = actual_buffer_count - outheight;
    if (screens[screen].scroll_up < 0)
        screens[screen].scroll_up = 0;
    int start = max(0, actual_buffer_count - outheight);
    int displayable = actual_buffer_count - start;
    for (i = start; i < actual_buffer_count; i++) {
        wmove(outwin, i - start + outheight - displayable, 0);
        int index = i;
        index -= screens[screen].buffer_count - 1; // replace the last buffer with
        index += screens[screen].buffer_index; // the selected one
        index -= screens[screen].scroll_up;
        index += MAX_BUFFER_SIZE * 2; // negative modulo prevention
        index %= MAX_BUFFER_SIZE;
        draw_colored(outwin, screens[screen].buffer[index], qfalse);
    }
    wrefresh(outwin);
}

static void draw_statuswin() {
    werase(statuswin);
    static char string[32];
    int i = 0;
    i += draw_colored(statuswin, " [", qtrue);
    timestring(string);
    wattron(statuswin, A_BOLD);
    i += draw_colored(statuswin, string, qtrue);
    wattroff(statuswin, A_BOLD);
    i += draw_colored(statuswin, "] ", qtrue);
    if (screens[screen].last_name != NULL) {
        wattron(statuswin, A_BOLD);
        i += draw_colored(statuswin, screens[screen].last_name, qtrue);
        wattroff(statuswin, A_BOLD);
        i += draw_colored(statuswin, " ", qtrue);
    }
    char number[2];
    qboolean first = qtrue;
    int j;
    for (j = 0; j < SCREENS; j++) {
        if (screens[j].updated) {
            if (first) {
                i += draw_colored(statuswin, "[Act: ", qtrue);
                first = qfalse;
            } else {
                i += draw_colored(statuswin, ",", qtrue);
            }
            sprintf(number, "%d", j);
            if (screens[j].important)
                wattron(statuswin, A_BOLD);
            i += draw_colored(statuswin, number, qtrue);
            if (screens[j].important)
                wattroff(statuswin, A_BOLD);
        }
    }
    if (!first)
        i += draw_colored(statuswin, "] ", qtrue);
    sprintf(number, "%d", screen);
    i += draw_colored(statuswin, "[", qtrue);
    wattron(statuswin, A_BOLD);
    i += draw_colored(statuswin, number, qtrue);
    wattroff(statuswin, A_BOLD);
    i += draw_colored(statuswin, "] ", qtrue);
    for (; i < COLS; i++)
        waddch(statuswin, ' ');
    wrefresh(statuswin);
}

void draw_status(int client, char *name) {
    screens[client + 1].last_name = name;
    draw_statuswin();
}

static void draw_inwin() {
    werase(inwin);
    wattrset(inwin, COLOR_PAIR(7));
    wattron(inwin, A_BOLD);
    draw_colored(inwin, "> ", qfalse);
    wattroff(inwin, A_BOLD);
    if (screens[screen].commandline_length < 1 || screens[screen].commandline[0] != '/')
        wattrset(inwin, COLOR_PAIR(2));
    draw_colored(inwin, screens[screen].commandline, qfalse);
    waddch(inwin, '_');
    wrefresh(inwin);
}

static void redraw() {
    draw_titlewin();
    draw_outwin();
    draw_statuswin();
    draw_inwin();
}

void set_screen(int new_screen) {
    screen = new_screen;
    screens[screen].updated = qfalse;
    screens[screen].important = qfalse;
    redraw();
}

void ui_output_real(int client, char *string) {
    int len = strlen(string);
    int i;
    int maxlen = min(COLS, MAX_OUTPUT_LENGTH - 2);
    qboolean empty = qfalse;
    if (screens[client + 1].ghost_line)
        screens[client + 1].ghost_line = qfalse;
    if (screens[client + 1].buffer_count == 0)
        screens[client + 1].buffer_count++;
    for (i = 0; i < len; i++) {
        if (screens[client + 1].next_line) {
            if (screens[client + 1].buffer_count < MAX_BUFFER_SIZE)
                screens[client + 1].buffer_count++;
            screens[client + 1].buffer_index = (screens[client + 1].buffer_index + 1) % MAX_BUFFER_SIZE;
            if (screens[client + 1].scroll_up)
                screens[client + 1].scroll_up++;
            screens[client + 1].output_length = 0;
            screens[client + 1].next_line = qfalse;
        }
        if (string[i] == '\n' || screens[client + 1].output_length > maxlen) {
            screens[client + 1].buffer[screens[client + 1].buffer_index][screens[client + 1].output_length] = '\0';
            screens[client + 1].next_line = qtrue;
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
            screens[client + 1].allow_time = string[i] == '\n';
            screens[client + 1].ghost_line = !empty || !screens[client + 1].allow_time;
        } else {
            if (screens[client + 1].ghost_line && screens[client + 1].output_length == 0) {
                int j;
                for (j = 0; j < min(6, COLS - 2); j++)
                    screens[client + 1].buffer[screens[client + 1].buffer_index][screens[client + 1].output_length++] = ' ';
                screens[client + 1].allow_time = qfalse;
            }
            screens[client + 1].allow_time = empty;
            screens[client + 1].buffer[screens[client + 1].buffer_index][screens[client + 1].output_length++] = string[i];
        }
    }
    screens[client + 1].ghost_line = empty;
}

void ui_output(int client, char *format, ...) {
    static char string[65536];
    string[0] = '^';
    string[1] = '7';
    int len = 2;
    if (screens[client + 1].allow_time) {
        len += timestring(string + len);
        string[len++] = ' ';
    }

	va_list	argptr;
	va_start(argptr, format);
    vsprintf(string + len, format, argptr);
	va_end(argptr);
    ui_output_real(client, string);
    draw_outwin();
    if (client + 1 != screen) {
        screens[client + 1].updated = qtrue;
        draw_statuswin();
    }
}

void ui_set_important(int client) {
    if (screens[client + 1].updated) {
        screens[client + 1].important = qtrue;
        draw_statuswin();
    }
}

static void screen_init(screen_t *s) {
    s->last_name = NULL;

    s->buffer_count = 0;
    s->ghost_line = qfalse;
    s->buffer_index = 0;
    s->output_length = 0;
    s->scroll_up = 0;
    s->next_line = qfalse;
    s->allow_time = qtrue;

    s->commandline_length = 0;

    s->updated = qfalse;
    s->important = qfalse;
}

static char suggestions[MAX_CMDS][MAX_SUGGESTION_SIZE];
static int suggestion_count;
static int suggesting_offset;

static void apply_suggestions(qboolean add_space) {
    qsort(suggestions, suggestion_count, MAX_SUGGESTION_SIZE, insensitive_cmp);
    int skip = 0;
    char *last = NULL;
    int i;
    for (i = 0; i + skip < suggestion_count; i++) {
        if (last != NULL && strcmp(suggestions[i], last))
            skip++;
        if (skip > 0 && i + skip < suggestion_count)
            strcpy(suggestions[i], suggestions[i + skip]);
        last = suggestions[i];
    }
    suggestion_count -= skip;

    if (suggestion_count == 0)
        return;

    screens[screen].commandline_length = suggesting_offset;
    qboolean valid = qtrue;
    for (i = 0; valid; i++) {
        char c = '\0';
        int j;
        for (j = 0; j < suggestion_count; j++) {
            if (i >= strlen(suggestions[j]) || (c != '\0' && suggestions[j][i] != c))
                valid = qfalse;
            c = suggestions[j][i];
        }
        if (c == '\0')
            valid = qfalse;
        if (valid)
            screens[screen].commandline[screens[screen].commandline_length++] = c;
    }
    if (suggestion_count == 1) {
        if (add_space)
            screens[screen].commandline[screens[screen].commandline_length++] = ' ';
    } else {
        ui_output(screen - 1, "\n^5Possibilities:\n");
        for (i = 0; i < suggestion_count; i++)
            ui_output(screen - 1, "%s\n", suggestions[i]);
    }
    screens[screen].commandline[screens[screen].commandline_length] = '\0';
}

static void complete_command() {
    suggesting_offset = 1;
    suggestion_count = cmd_suggest(screen - 1, screens[screen].commandline + suggesting_offset, suggestions);
    apply_suggestions(qtrue);
}

static void complete_chat() {
    if (screen == 0)
        return;

    for (suggesting_offset = screens[screen].commandline_length - 1; suggesting_offset >= 0; suggesting_offset--) {
        if (screens[screen].commandline[suggesting_offset] == ' ')
            break;
    }
    suggesting_offset++;

    suggestion_count = player_suggest(screen - 1, screens[screen].commandline + suggesting_offset, suggestions);
    apply_suggestions(qfalse);
}

void ui_run() {
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
    wtimeout(inwin, INPUT_TIME);

    client_register_commands();
    int i;
    for (i = 0; i < SCREENS; i++) {
        screen_init(screens + i);
        if (i > 0)
            client_start(i - 1);
    }

    set_title(-1, NULL, NULL, NULL, NULL, NULL);
    draw_outwin();
    draw_statuswin();
    draw_inwin();

    qboolean alt = qfalse;
    for (;;) {
        if (stopped)
            break;
        for (i = 0; i < CLIENT_SCREENS; i++)
            client_frame(i);
        int c = wgetch(inwin);
        if (c == -1)
            continue;
        if (alt) {
            if (c >= '0' && c <= '9')
                set_screen(c - '0');
            alt = qfalse;
            continue;
        }
        bool handled = TRUE;
        switch (c) {
            case KEY_PPAGE:
                screens[screen].scroll_up += SCROLL_SPEED;
                break;
            case KEY_NPAGE:
                screens[screen].scroll_up -= SCROLL_SPEED;
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
            case 27:
                alt = qtrue;
                continue;
            case KEY_BACKSPACE:
            case 127:
                if (screens[screen].commandline_length > 0)
                    screens[screen].commandline_length--;
                screens[screen].commandline[screens[screen].commandline_length] = '\0';
                break;
            case 9:
                if (screens[screen].commandline[0] == '/')
                    complete_command();
                else
                    complete_chat();
                break;
            case KEY_ENTER:
            case 13:
                screens[screen].commandline[screens[screen].commandline_length] = '\0';
                screens[screen].scroll_up = 0;
                int old_screen = screen;
                if (screens[screen].commandline_length > (screens[screen].commandline[0] == '/' ? 1 : 0)) {
                    if (screens[screen].commandline[0] == '/') {
                        ui_output(screen - 1, "%s\n", screens[screen].commandline);
                        cmd_execute(screen - 1, screens[screen].commandline + 1);
                    } else {
                        if (screen == 0)
                            ui_output(screen - 1, "^5If you really want to broadcast your message, invoke /say manually.\n");
                        else
                            client_command(screen - 1, "say %s", screens[screen].commandline);
                    }
                }
                screens[old_screen].commandline_length = 0;
                screens[old_screen].commandline[screens[old_screen].commandline_length] = '\0';
                draw_outwin();
                break;
            default:
                screens[screen].commandline[screens[screen].commandline_length++] = c;
                screens[screen].commandline[screens[screen].commandline_length] = '\0';
                break;
        }
        draw_inwin();
    }

    quit();
}
