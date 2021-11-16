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

#ifndef CONSOLE_H
#define CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Colors
 */
#define BLACK   0
#define RED     1
#define GREEN   2
#define YELLOW  3
#define BLUE    4
#define MAGENTA 5
#define CYAN    6
#define GRAY    7
#define INTENSE 8

/*
 * Key codes
 */
#define LEFT_ARROW  128
#define RIGHT_ARROW 129
#define UP_ARROW    130
#define DOWN_ARROW  131
#define OTHER_KEY   255

/*
 * Functions
 */

#ifdef __GNUC__
#  define CONS_PRINTW_ATTR __attribute__ ((format (printf, 1, 2)))
#else
#  define CONS_PRINTW_ATTR
#endif

void cons_clear_screen(void);
void cons_move_cursor(int row, int col);
void cons_change_color(int fg_color, int bg_color);
int cons_get_screen_height(void);
int cons_get_screen_width(void);
int cons_get_keypress(void);
int cons_wait_for_keypress(void);
void cons_sleep_ms(int ms);
void cons_printw(const char *fmt, ...) CONS_PRINTW_ATTR;
void cons_update(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

