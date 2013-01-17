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
#include <curses.h>
#include <signal.h>

static WINDOW *mainwin;
static WINDOW *outwin;
static WINDOW *statuswin;
static WINDOW *inwin;

static char commandline[500];
static int commandline_length = 0;

static void finish(int sig) {
    endwin();
    exit(sig);
}

static void init_colors() {
    if (has_colors()) {
        start_color();

        init_pair(0, COLOR_BLACK, COLOR_BLACK);
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);
        init_pair(8, COLOR_YELLOW, COLOR_BLACK);
        init_pair(9, COLOR_WHITE, COLOR_BLACK);
        init_pair(10, COLOR_BLACK, COLOR_WHITE);
    }
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
    waddstr(inwin, ">");
    wattroff(inwin, A_BOLD);
    int i;
    for (i = 0; i < commandline_length; i++)
        waddch(inwin, commandline[i]);
    if (refresh)
        wrefresh(inwin);
}

void ui_start() {
    signal(SIGINT, finish);
    signal(SIGSEGV, finish);

    mainwin = initscr();
    nonl();
    cbreak();
    noecho();

    init_colors();

    outwin = subwin(mainwin, LINES - 2, COLS, 0, 0);
    scrollok(outwin, TRUE);
    wattron(outwin, A_BOLD);

    statuswin = subwin(mainwin, 1, COLS, LINES - 2, 0);
    wattrset(statuswin, COLOR_PAIR(10));

    inwin = subwin(mainwin, 1, COLS, LINES - 1, 0);
    keypad(inwin, TRUE);

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
                commandline_length = 0;
                break;
            default:
                commandline[commandline_length++] = c;
                break;
        }
        draw_inwin(TRUE);
    }

    finish(0);
}
