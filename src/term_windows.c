// term_windows.c - A terminal windows library
// Copyright (C) 2020  Nicklas Carpenter

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <curses.h>
#include <locale.h>
#include <limits.h>
#include <stdlib.h>
#include <term_windows.h>
#include <string.h>

// TODO Create window registration
// TODO Verify license stuff
// TODO Abstract window messages to memory buffers
// Currently, messages are sent directly to the console. Since the size of the
// console is variable, this creates a problem if things change. Additionally,
// it becomes hard to determine how many lines we need to reserve when printing
// a message (e.g. should we scroll down three lines). The logic of sending a
// message to a window and displaying all the messages in a window should be
// decoupled and handled seperately

void term_windows_init() {
    // Set our locale to be portable, since our ncurses instance will inherit
    // this
    setlocale(LC_ALL, "");

    // Initialqize curses in standard mode
    initscr(); // Intialize our screen; we need to do this before anything else
    cbreak(); // Disable line-buffering (and buffering in general)
    noecho(); // Disable echoing of characters read into stdin
    nonl(); // Do not automatically read a return key press as a newline0
    // Disable flushing of the tty output driver when an interrupt key is 
    // pressed (preventing curses from losing its place)
    intrflush(stdscr, FALSE);
    // Use single values for function keys
    // (making it easier to read key inputs)
    keypad(stdscr, TRUE);
}

void term_windows_end() {
    // Reset all terminal input and output options
    nocbreak();
    echo();
    nl();

    // end curses
    endwin();
}

ext_window *ext_window_create(uint16_t nlines, uint16_t ncols, uint16_t start_row, uint16_t start_col) {
    ext_window *new_ext_window = malloc(sizeof(ext_window));
    
    new_ext_window->window = newwin(nlines, ncols, start_row, start_col);
    new_ext_window->ncols = ncols;
    new_ext_window->nlines = nlines;

    return new_ext_window;
}

ext_window *ext_window_create_from_existing(WINDOW *win) {
    ext_window *new_ext_window = malloc(sizeof(ext_window));

    new_ext_window->window = win;
    getmaxyx(win, new_ext_window->nlines, new_ext_window->ncols);
    new_ext_window->nlines++;
    new_ext_window->ncols++;

    return new_ext_window;
}

edit_window *edit_window_create(uint16_t nlines, uint16_t ncols, uint16_t start_row, uint16_t start_col) {
    edit_window *new_edit_window = malloc(sizeof(edit_window));

    new_edit_window->window = newwin(nlines, ncols, start_row, start_col);
    new_edit_window->ncols = ncols;
    new_edit_window->nlines = nlines;

    new_edit_window->print_curs = malloc(sizeof(cursor));
    new_edit_window->print_curs->cur_line = 0;
    new_edit_window->print_curs->cur_col = 0;

    return new_edit_window;
}

// TODO Handle case where we try to move up too many lines
int64_t edit_window_move_v(edit_window *win, int64_t nlines) {
    uint64_t displacement = nlines;

    // We are moving past the last visible line. Time to scroll.
    if (win->print_curs->cur_line + nlines > win->nlines - 1) {
        displacement = win->nlines - win->print_curs->cur_line;
    }
    else if (win->print_curs->cur_line + nlines < 0) {
        displacement = -(win->print_curs->cur_col);
    }

    win->print_curs->cur_line += displacement;

    wmove(win->window, win->print_curs->cur_line, win->print_curs->cur_col);
    wrefresh(win->window);

    return displacement;
}

int32_t edit_window_move_h(edit_window *win, int32_t ncols) {
    int32_t displacement;
    uint8_t max_disp = win->ncols - win->print_curs->cur_col;

    if (ncols > max_disp) {
        displacement = max_disp;
    }
    else if (win->print_curs->cur_col - ncols < 0) {
        displacement = -(win->print_curs->cur_col);
    }
    else {
        displacement = ncols;
    }

    wmove(win->window, win->print_curs->cur_line, win->print_curs->cur_col + displacement);
    wrefresh(win->window);

    return displacement;
}

int8_t edit_window_set_row(edit_window *win, uint16_t line_num) {
    if (line_num >= 0 && line_num <= win->nlines) {
        win->print_curs->cur_line = line_num;
        wmove(win->window, line_num, win->print_curs->cur_col);
        wrefresh(win->window);

        return 0;
    }

    return -1;
}

int8_t edit_window_set_col(edit_window *win, uint16_t col_num) {
    if (col_num >= 0 && col_num <= win->ncols) {
        win->print_curs->cur_col = col_num;
        wmove(win->window, col_num, win->print_curs->cur_col);
        wrefresh(win->window);

        return 0;
    }

    return -1;
}

int8_t edit_window_clrln(edit_window *win) {
    wclrtoeol(win->window);
    wrefresh(win->window);

    return 0;
}

int8_t edit_window_putc(edit_window *win, char c) {
    waddch(win->window, c);
    wrefresh(win->window);

    win->print_curs->cur_col++;

    return 0;
}

int8_t edit_window_backspace(edit_window *win) {
    if (win->print_curs->cur_col > 0) {
        wmove(win->window, win->print_curs->cur_line, --(win->print_curs->cur_col));

        // Delete the character present)
        wdelch(win->window);

        wrefresh(win->window);

        return 0;
    }

    return -1;
}

msg_window *msg_window_create(uint16_t nlines, uint16_t ncols, uint16_t start_row, uint16_t start_col) {
    msg_window *new_msg_window = malloc(sizeof(msg_window));

    new_msg_window->window = newwin(nlines, ncols, start_row, start_col);
    scrollok(new_msg_window->window, TRUE);
    new_msg_window->nlines = nlines;
    new_msg_window->ncols = ncols;

    new_msg_window->read_curs = malloc(sizeof(cursor));
    new_msg_window->read_curs->cur_line = 0;
    new_msg_window->read_curs->cur_col = 0;

    new_msg_window->print_curs = malloc(sizeof(cursor));
    new_msg_window->print_curs->cur_line = 0;
    new_msg_window->print_curs->cur_col = 0;

    return new_msg_window;
}

// TODO Handle case where we try to move up too many lines
int64_t msg_window_move_v(msg_window *win, int64_t nlines) {
    uint64_t displacement = nlines;

    // We are moving past the last visible line. Time to scroll.
    if (win->print_curs->cur_line + nlines > win->nlines - 1) {
        msg_window_scroll(win, win->print_curs->cur_line + nlines - (win->nlines - 1));
    }
    else if (win->print_curs->cur_line + nlines < 0) {
        displacement = -(win->print_curs->cur_col);
    }

    win->print_curs->cur_line += displacement;

    wmove(win->window, win->print_curs->cur_line, win->print_curs->cur_col);
    wrefresh(win->window);

    return displacement;
}

int32_t msg_window_move_h(msg_window *win, int32_t ncols) {
    int32_t displacement;
    uint8_t max_disp = (win->ncols - 1) - win->print_curs->cur_col;

    if (ncols > max_disp) {
        displacement = max_disp;
    }
    else if (win->print_curs->cur_col - ncols < 0) {
        displacement = -(win->print_curs->cur_col);
    }
    else {
        displacement = ncols;
    }

    win->print_curs->cur_col += displacement;

    wmove(win->window, win->print_curs->cur_line, win->print_curs->cur_col);
    wrefresh(win->window);

    return displacement;
}

int8_t msg_window_set_row(msg_window *win, uint16_t line_num) {
    if (line_num >= 0 && line_num <= win->nlines) {
        win->print_curs->cur_line = line_num;
        wmove(win->window, line_num, win->print_curs->cur_col);
        wrefresh(win->window);

        return 0;
    }

    return -1;
}

int8_t msg_window_set_col(msg_window *win, uint16_t col_num) {
    if (col_num >= 0 && col_num <= win->ncols) {
        win->print_curs->cur_col = col_num;
        wmove(win->window, win->print_curs->cur_line, col_num);
        wrefresh(win->window);

        return 0;
    }

    return -1;
}

// TODO support scrolling upwards. See comment on top regarding backing store
int8_t msg_window_scroll(msg_window* win, uint64_t nlines) {
    wscrl(win->window, nlines);
    win->print_curs->cur_line -= nlines;

    return 0;
}

int8_t msg_window_puts(msg_window *win, char* str) {
    waddstr(win->window, str);
    wrefresh(win->window);

    win->print_curs->cur_col += strlen(str);

    return 0;
}