// term_windows.h - Contains defintions for the term_windows API

// Currently, term_windows is a wrapper for ncurses, but the ultimate goal
// is use terminfo or a library that interfaces with it; ncurses has a strange
// interface, hence this wrapper

// TODO Note caveats about msg_window_move_v and edit_window_move_v
// Notably, msg_window_move_v attemtps to scroll.

#ifndef CURSES_TEST_H
#define CURSES_TEST_H

#include <stdint.h>
#include <curses.h>

// Define the character codes for some common keypresses that ncurses does not
// include
#define KEY_ALT_BACKSPACE 127
#define KEY_ALT_ENTER_1 10
#define KEY_ALT_ENTER_2 13
#define ESC_SEQUENCE_START 27
#define KEY_UP_ARROW 65
#define KEY_DOWN_ARROW 66
#define KEY_RIGHT_ARROW 67
#define KEY_LEFT_ARROW 68
#define PAGE_SIZE 4096

// A cursor is an active location on a window; it is always associated with
// a window. Cursors have multiple purposes e.g.
//      print_curs: A cursor used to determine where to add text in a window
//      read_curs: A cursor used to control the view of the window (e.g. the
//      visible text)
typedef struct cursor {
    uint8_t cur_line;
    uint8_t cur_col;
} cursor;

// Note for the following window definitions:
// Technically, a ncurses WINDOW struct
// contains size information, but it is stored here for compatibility reasons
// (ncurses specifies functions for window size information, so there is no
// gaurantee of specific implementation for the window struct)

// An ext_window (extended window) is a simple base window
// An ext_window currently encompasses a ncurses WINDOW
// pointer along with the window size information
typedef struct ext_window {
    WINDOW *window;

    uint16_t nlines;
    uint16_t ncols;
} ext_window;

// A msg_window (message window) is a viewing window where messages are
// printed and displayed
// A msg_window currently encompasses a ncurses WINDOW
// pointer, window size information, and seperate cursors for reading
// (read_curs) and editing (print_curs)
typedef struct msg_window {
    WINDOW *window;

    uint16_t nlines;
    uint16_t ncols;

    cursor *read_curs;
    cursor *print_curs;
} msg_window;

// An edit_window is a window for live text editing
// A msg_window currently encompasses a ncurses WINDOW
// pointer, window size information, and seperate cursors for reading
// (read_curs) and editing (print_curs)
typedef struct edit_window {
    WINDOW *window;
    
    uint8_t nlines;
    uint8_t ncols;

    cursor *print_curs;
} edit_window;

void term_windows_init();
void term_windows_end();

// Creates a new ext_window with the given size parameters. Returns a pointer
// to the newly created window
ext_window *ext_window_create(uint16_t nlines, uint16_t ncols, uint16_t start_row, uint16_t start_col);

// Creates a new ext_window based on a pre-existing ncurses window. Returns a
// pointer to the newly created ext_window
// Used primarily to create a wrapper for ncurses stdscr
ext_window *ext_window_create_from_existing(WINDOW *win);

// Creates a new edit_window with the given size parameters. Returns a pointer
// to the newly edit_created window
edit_window *edit_window_create(uint16_t nlines, uint16_t ncols, uint16_t start_row, uint16_t start_col);

// Moves the edit_window's print cursor down nlines lines. Returns the number
// of lines the cursor was moved; this may differ from nlines if the distance
// between the current line and the last line is greated tham nlines. If nlines
// is negative, the cursor is move upwards |nlines| lines
int64_t edit_window_move_v(edit_window *win, int64_t nlines);

// Moves the msg_window's print cursor right ncols columns. Returns the number
// of columns the cursor was moved; this may differ from ncols if the distance
// between the current column and the last column is greated than ncols. If
// ncols is negative, the cursor is move upwards |ncols| columns
int32_t edit_window_move_h(edit_window *win, int32_t ncols);

// Sets the line of the edit_windows print cursor to line_num. Returns 0 upon
// success or -1 if the given line_num is out of the edit_window bounds.
int8_t edit_window_set_row(edit_window *win, uint16_t line_num);

// Sets the column of the edit_windows print cursor to col_num. Returns 0 upon
// success or 1 if the given col_num is out of the msg_window bounds.
int8_t edit_window_set_col(edit_window *win, uint16_t col_num);

// Writes a character to the edit_window at the current position of the
// print cursor
int8_t edit_window_putc(edit_window *win, char c);

// Backspaces the last character in the current line of the edit_window
int8_t edit_window_backspace(edit_window *win);

// Clears the current line of the edit_window
int8_t edit_window_clrln(edit_window *win);

// Creates a new msg_window with the given size parameters. Returns a pointer
// to the newly msg_created window
msg_window *msg_window_create(uint16_t nlines, uint16_t ncols, uint16_t start_row, uint16_t start_col);

int64_t msg_window_move_v(msg_window *win, int64_t nlines);

// Moves the msg_window's print cursor right ncols columns. Returns the number
// of columns the cursor was moved; this may differ from ncols if the distance
// between the current column and the last column is greated than ncols. If
// ncols is negative, the cursor is move upwards |ncols| columns
int32_t msg_window_move_h(msg_window *win, int32_t ncols);

// Sets the line of the msg_windows print cursor to line_num. Returns 0 upon
// success or -1 if the given line_num is out of the msg_window bounds.
int8_t msg_window_set_row(msg_window *win, uint16_t line_num);

// Sets the column of the msg_windows print cursor to col_num. Returns 0 upon
// success or -1 if the given col_num is out of the msg_window bounds.
int8_t msg_window_set_col(msg_window *win, uint16_t col_num);

// Scrolls the msg_window down nlines. This has the effect that the contents on
// line n will be moved to line n - nlines. The first nlines lines will be
// shifted out of the top of the window and nlines blank lines will be shifted
// in at the bottom of the window. It is currently not possible to scroll up.
// Returns 0 on success or >0 on error
int8_t msg_window_scroll(msg_window *win, uint64_t nlines);

// Prints a message to msg_window starting starting at the position of the
// print cursor
int8_t msg_window_puts(msg_window *win, char *str);

#endif