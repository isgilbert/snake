/*
 * Terminal graphics library
 * Copyright (c) 2007-2010 David H. Hovemeyer <daveho@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "Console.h"

/* Figure out what platform we're on */
#ifdef _MSC_VER
# define CONS_WIN
#else
# define CONS_NCURSES
#endif

/*----------------------------------------------------------------------
 * OS-dependent functions (Linux/Unix/Cygwin)
 *----------------------------------------------------------------------*/

#ifdef CONS_NCURSES

#ifdef CONS_NCURSES_IS_IN_NCURSES_INCLUDE_DIR
#  include <ncurses/ncurses.h>
#else
#  include <ncurses.h>
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define ESCAPE      27      /* ASCII code for ESC */

#define KQUEUE_SIZE 4

#define KNORMAL     0       /* Normal character processing */
#define KESC        1       /* Saw escape character */
#define KLBRACKET   2       /* Saw escape + [ */
#define KDRAIN      3       /* Draining characters following unknown escape sequence */

static int s_init;
static int s_ncolors;
static int s_curfg, s_curbg;
static int s_height, s_width;
static int s_pair[8][8];
static int s_pairs_created;
static int s_khead, s_ktail;
static unsigned char s_kqueue[KQUEUE_SIZE];
static int s_kstate;

static int osdep_kqueue_next(int i)
{
	return (i + 1) % KQUEUE_SIZE;
}

static int osdep_kqueue_is_empty(void)
{
	return s_khead == s_ktail;
}

static void osdep_kqueue_enqueue(unsigned char ch)
{
	assert(osdep_kqueue_next(s_ktail) != s_khead); /* make sure queue is not full */
	s_kqueue[s_ktail] = ch;
	s_ktail = osdep_kqueue_next(s_ktail);
}

static unsigned char osdep_kqueue_dequeue(void)
{
	unsigned char ch;

	assert(!osdep_kqueue_is_empty());
	ch = s_kqueue[s_khead];
	s_khead = osdep_kqueue_next(s_khead);

	return ch;
}

static void osdep_atexit_handler(void)
{
	if (s_init) {
		endwin();
#ifdef CONS_DEBUG
		printf("%d color pairs available\n", s_ncolors);
		printf("%d color pairs created\n", s_pairs_created);

		{
			int j, i;
			for (j = 0; j < 8; j++) {
				for (i = 0; i < 8; i++) {
					printf("%02i ", s_pair[j][i]);
				}
				printf("\n");
			}
		}
#endif

		// restore blocking input
		fcntl(STDIN_FILENO, F_SETFL, 0);
	}
}

static short osdep_to_curses_color(int color)
{
	switch (color)
	{
	case BLACK:
		return COLOR_BLACK;
	case RED:
		return COLOR_RED;
	case GREEN:
		return COLOR_GREEN;
	case YELLOW:
		return COLOR_YELLOW;
	case BLUE:
		return COLOR_BLUE;
	case MAGENTA:
		return COLOR_MAGENTA;
	case CYAN:
		return COLOR_CYAN;
	case GRAY:
		return COLOR_WHITE;
	default:
		assert(0);
		return COLOR_BLACK;
	}
}

static void osdep_init_colors(void)
{
	if (has_colors()) {
		int fg, bg;
		int next_pair = 1;

		start_color();
		s_ncolors = COLOR_PAIRS;

		for (bg = 0; bg < 8; bg++) {
			for (fg = 0; fg < 8; fg++) {
				short pair;

				if ((bg == BLACK && fg == GRAY) || next_pair >= s_ncolors) {
					pair = 0;
				} else {
					pair = next_pair;
					next_pair++;
					init_pair(pair, osdep_to_curses_color(fg), osdep_to_curses_color(bg));
					s_pairs_created++;
				}

				s_pair[bg][fg] = pair;
			}
		}
	}
}

static void osdep_cons_init(void)
{
	if (!s_init) {
		/*
		 * Initialize display.
		 */
		initscr();
		osdep_init_colors();
		s_curfg = GRAY;
		s_curbg = BLACK;
		cbreak();
		noecho();
		nonl();
		getmaxyx(stdscr, s_height, s_width);
		atexit(&osdep_atexit_handler);

		/*
		 * Initialize keyboard.
		 */
		fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
		s_khead = 0;
		s_ktail = 0;
		s_kstate = KNORMAL;

		s_init = 1;
	}
}

static void osdep_clear_screen(void)
{
	erase();
}

static void osdep_move_cursor(int row, int col)
{
	if (row < 0) {
		row = 0;
	} else if (row >= s_height) {
		row = s_height - 1;
	}

	if (col < 0) {
		col = 0;
	} else if (col >= s_width) {
		col = s_width - 1;
	}
	move(row, col);
}

static void osdep_sleep_ms(int ms)
{
	fd_set r, w, x;
	struct timeval delay;

	FD_ZERO(&r);
	FD_ZERO(&w);
	FD_ZERO(&x);
	delay.tv_sec = ms / 1000;
	delay.tv_usec = (ms % 1000) * 1000;

	select(0, &r, &w, &x, &delay);
}

static void osdep_change_color(int fg_color, int bg_color)
{
	s_curfg = fg_color;
	s_curbg = bg_color;
}

static int osdep_get_screen_height(void)
{
	return s_height;
}

static int osdep_get_screen_width(void)
{
	return s_width;
}

static int osdep_get_keypress(void)
{
	unsigned char ch;

start:
	if (s_kstate == KDRAIN) {
		assert(!osdep_kqueue_is_empty());
		ch = osdep_kqueue_dequeue();
		s_khead = osdep_kqueue_next(s_khead);
		if (osdep_kqueue_is_empty()) {
			s_kstate = KNORMAL;
		}
		return ch;
	}

readkey:
	if (read(STDIN_FILENO, &ch, 1) < 0) {
		return -1;
	}

	if (s_kstate == KNORMAL) {
		if (ch == ESCAPE) {
			osdep_kqueue_enqueue(ch);
			s_kstate = KESC;
			goto readkey;
		} else {
			return ch;
		}
	} else if (s_kstate == KESC) {
		osdep_kqueue_enqueue(ch);
		if (ch == '[') {
			s_kstate = KLBRACKET;
			goto readkey;
		} else {
			/* unknown escape code */
			s_kstate = KDRAIN;
			goto start;
		}
	} else if (s_kstate == KLBRACKET) {
		int key;
		switch (ch) {
		case 'A':
			key = UP_ARROW;
			break;
		case 'B':
			key = DOWN_ARROW;
			break;
		case 'C':
			key = RIGHT_ARROW;
			break;
		case 'D':
			key = LEFT_ARROW;
			break;

		default:
			osdep_kqueue_enqueue(ch);
			s_kstate = KDRAIN;
			goto start;
		}

		s_khead = 0;
		s_ktail = 0;
		s_kstate = KNORMAL;
		return key;
	} else {
		assert(0);
		return -1;
	}
}

static int osdep_wait_for_keypress(void)
{
	int c;

	fcntl(STDIN_FILENO, F_SETFL, 0);          // blocking input
	c = osdep_get_keypress();
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // nonblocking input
	
	return c;
}

static void osdep_printw(const char *fmt, va_list args)
{
	int fg, bg;

	fg = s_curfg & 7;
	bg = s_curbg & 7;

	assert(fg >= 0 && fg <= 7);
	assert(bg >= 0 && bg <= 7);

	attron(COLOR_PAIR(s_pair[bg][fg]));
	if (s_curfg & INTENSE) {
		attron(A_BOLD);
	}
	vwprintw(stdscr, fmt, args);
	attroff(COLOR_PAIR(s_pair[bg][fg]));
	if (s_curfg & INTENSE) {
		attroff(A_BOLD);
	}
	
}

static void osdep_cons_update(void)
{
	refresh();
}

#endif /* CONS_NCURSES */

/*----------------------------------------------------------------------
 * OS-dependent functions (Windows)
 *----------------------------------------------------------------------*/

#ifdef CONS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define VK_0 0x30
#define VK_9 0x39
#define VK_A 0x41
#define VK_Z 0x5A

static HANDLE s_cons;
static HANDLE s_cons_in;
static CONSOLE_SCREEN_BUFFER_INFO s_info;
static int s_cursX;
static int s_cursY;
static WORD s_curAttr;
static CHAR_INFO *s_scrnBuf;
static COORD s_bufSize;
static COORD s_origin;

static void osdep_cons_init(void)
{
	if (s_cons == NULL) {
		s_cons = (HANDLE) GetStdHandle(STD_OUTPUT_HANDLE);
		s_cons_in = (HANDLE) GetStdHandle(STD_INPUT_HANDLE);

		if (!GetConsoleScreenBufferInfo(s_cons, &s_info)) {
			fprintf(stderr, "Could not get ConsoleScreenBufferInfo\n");
			exit(1);
		}

		/* find out how large the window is */
		s_bufSize.X = (s_info.srWindow.Right - s_info.srWindow.Left) + 1;
		s_bufSize.Y = (s_info.srWindow.Bottom - s_info.srWindow.Top) + 1;

		/* allocate off-screen buffer */
		s_scrnBuf = malloc(sizeof(CHAR_INFO) * s_bufSize.Y * s_bufSize.X);

		/* set initial cursor position/attributes (based on current) */
		s_cursX = s_info.dwCursorPosition.X;
		s_cursY = s_info.dwCursorPosition.Y;
		s_curAttr = s_info.wAttributes;

		/* copy current screen contents into screen buffer */
		ReadConsoleOutput(s_cons, s_scrnBuf, s_bufSize, s_origin, &s_info.srWindow);
	}
}

static int osdep_get_screen_height(void)
{
	return (int) (s_info.srWindow.Bottom - s_info.srWindow.Top) + 1;
}

static int osdep_get_screen_width(void)
{
	return (int) (s_info.srWindow.Right - s_info.srWindow.Left) + 1;
}

static WORD osdep_translate_color(int fg_color, int bg_color)
{
	WORD result;

	result = 0;

	if (fg_color & RED) {
		result |= FOREGROUND_RED;
	}
	if (fg_color & GREEN) {
		result |= FOREGROUND_GREEN;
	}
	if (fg_color & BLUE) {
		result |= FOREGROUND_BLUE;
	}
	if (fg_color & INTENSE) {
		result |= FOREGROUND_INTENSITY;
	}

	if (bg_color & RED) {
		result |= BACKGROUND_RED;
	}
	if (bg_color & GREEN) {
		result |= BACKGROUND_GREEN;
	}
	if (bg_color & BLUE) {
		result |= BACKGROUND_BLUE;
	}
	if (bg_color & INTENSE) {
		result |= BACKGROUND_INTENSITY;
	}

	return result;
}

static int osdep_is_ascii(WORD wVirtualKeyCode)
{
	/* See:
	 *    http://api.farmanager.com/en/winapi/virtualkeycodes.html
	 *    http://delphi.about.com/od/objectpascalide/l/blvkc.htm
	 */
	return wVirtualKeyCode == VK_BACK
		|| wVirtualKeyCode == VK_TAB
		|| wVirtualKeyCode == VK_RETURN
		|| wVirtualKeyCode == VK_ESCAPE
		|| wVirtualKeyCode == VK_SPACE
		|| (wVirtualKeyCode >= VK_0 && wVirtualKeyCode <= VK_9)
		|| (wVirtualKeyCode >= VK_A && wVirtualKeyCode <= VK_Z)
		|| (wVirtualKeyCode >= VK_OEM_1 && wVirtualKeyCode <= VK_OEM_3)
		|| (wVirtualKeyCode >= VK_OEM_4 && wVirtualKeyCode <= VK_OEM_7);
}

static int osdep_translate_key_code(KEY_EVENT_RECORD rec)
{
	WORD wVirtualKeyCode;

	wVirtualKeyCode = rec.wVirtualKeyCode;

	if (osdep_is_ascii(wVirtualKeyCode)) {
		return rec.uChar.AsciiChar;
	} else if (wVirtualKeyCode == VK_LEFT) {
		return LEFT_ARROW;
	} else if (wVirtualKeyCode == VK_RIGHT) {
		return RIGHT_ARROW;
	} else if (wVirtualKeyCode == VK_UP) {
		return UP_ARROW;
	} else if (wVirtualKeyCode == VK_DOWN) {
		return DOWN_ARROW;
	} else {
		return OTHER_KEY;
	}
}

static void osdep_cons_scroll(void)
{
	int height = osdep_get_screen_height();
	int width = osdep_get_screen_width();
	int j;
	size_t line_size = width * sizeof(CHAR_INFO);

	s_cursY = height - 1;

	/*
	 * Copy each line (except the bottom line) up one row.
	 */
	for (j = width; j < width*height; j++) {
		s_scrnBuf[j - width] = s_scrnBuf[j];
	}

	/*
	 * Blank the bottom line using current attribute.
	 */
	for (j = 0; j < width; j++) {
		CHAR_INFO space;
		space.Attributes = s_curAttr;
		space.Char.UnicodeChar = (WCHAR) ' ';
		s_scrnBuf[((height-1) * width) + j] = space;
	}
}

static void osdep_cons_newline(void)
{
	int height = osdep_get_screen_height();
	s_cursX = 0;
	s_cursY++;
	if (s_cursY >= height) {
		osdep_cons_scroll();
	}
}

static void osdep_cons_put_char(int ch)
{
	int width = osdep_get_screen_width();
	int pos = (s_cursY * width) + s_cursX;

	if (ch == '\n') {
		osdep_cons_newline();
	} else {
		s_scrnBuf[pos].Attributes = s_curAttr;
		s_scrnBuf[pos].Char.AsciiChar = ch;

		s_cursX++;
		if (s_cursX >= width) {
			osdep_cons_newline();
		}
	}
}

static void osdep_clear_screen(void)
{
	int i, nchars;
	WORD fillAttr = osdep_translate_color(GRAY, BLACK);

	nchars = osdep_get_screen_height() * osdep_get_screen_width();
	for (i = 0; i < nchars; i++) {
		s_scrnBuf[i].Attributes = fillAttr;
		s_scrnBuf[i].Char.UnicodeChar = (WCHAR) ' ';
	}
}

static void osdep_move_cursor(int row, int col)
{
	int height, width;

	height = osdep_get_screen_height();
	width = osdep_get_screen_width();

	s_cursY = row;
	s_cursX = col;

	if (s_cursY < 0) {
		s_cursY = 0;
	}
	if (s_cursY >= height) {
		s_cursY = height - 1;
	}
	if (s_cursX < 0) {
		s_cursX = 0;
	}
	if (s_cursX >= width) {
		s_cursX = width - 1;
	}
}

static void osdep_change_color(int fg_color, int bg_color)
{
	s_curAttr = osdep_translate_color(fg_color, bg_color);
}

static int osdep_get_keypress(void)
{
	INPUT_RECORD rec;
	DWORD nRead;

	while (1) {
		if (!PeekConsoleInput(s_cons_in, &rec, 1, &nRead)) {
			/* error */
			return -1;
		}

		if (nRead == 0) {
			/* no events */
			return -1;
		}

		/* consume the event */
		ReadConsoleInput(s_cons_in, &rec, 1, &nRead);

		/* if it's a key down event, translate it and return it */
		if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
			return osdep_translate_key_code(rec.Event.KeyEvent);
		}
	}
}

static int osdep_wait_for_keypress(void)
{
	INPUT_RECORD rec;
	DWORD nRead;

	while (1) {
		if (!ReadConsoleInput(s_cons_in, &rec, 1, &nRead)) {
			/* error */
			return -1;
		}

		/* if it's a key down event, translate it and return it */
		if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
			return osdep_translate_key_code(rec.Event.KeyEvent);
		}
	}
}

static void osdep_sleep_ms(int ms)
{
	Sleep(ms);
}

static void osdep_printw(const char *fmt, va_list args)
{
	char buf[2000];
	int count, i;

	count = vsnprintf(buf, sizeof(buf), fmt, args);
	buf[sizeof(buf) - 1] = '\0';

	for (i = 0; i < count; i++) {
		osdep_cons_put_char(buf[i]);
	}
}

static void osdep_cons_update(void)
{
	COORD position;

	/* copy off-screen buffer to window */
	WriteConsoleOutput(s_cons, s_scrnBuf, s_bufSize, s_origin, &s_info.srWindow);

	/* update cursor position */
	position.X = s_cursX;
	position.Y = s_cursY;
	SetConsoleCursorPosition(s_cons, position);
}

#endif /* CONS_WIN */

/*----------------------------------------------------------------------
 * Public functions (OS-independent)
 *----------------------------------------------------------------------*/

void cons_clear_screen(void)
{
	osdep_cons_init();
	osdep_clear_screen();
}

void cons_move_cursor(int row, int col)
{
	osdep_cons_init();
	osdep_move_cursor(row, col);
}

void cons_change_color(int fg_color, int bg_color)
{
	osdep_cons_init();
	osdep_change_color(fg_color, bg_color);
}

int cons_get_screen_height(void)
{
	osdep_cons_init();

	return osdep_get_screen_height();
}

int cons_get_screen_width(void)
{
	osdep_cons_init();

	return osdep_get_screen_width();
}

int cons_get_keypress(void)
{
	osdep_cons_init();
	return osdep_get_keypress();
}

int cons_wait_for_keypress(void)
{
	osdep_cons_init();
	return osdep_wait_for_keypress();
}

void cons_sleep_ms(int ms)
{
	osdep_cons_init();
	osdep_sleep_ms(ms);
}

void cons_printw(const char *fmt, ...)
{
	va_list args;

	osdep_cons_init();

	va_start(args, fmt);
	osdep_printw(fmt, args);
	va_end(args);
}

void cons_update(void)
{
	osdep_cons_init();
	osdep_cons_update();
}

