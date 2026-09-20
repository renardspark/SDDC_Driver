/*
 * This file is part of SDDC_Driver.
 *
 * =====================
 *      MIT License
 * =====================
 *
 * Copyright (C) 2020 - Oscar Steila
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _UTIH32_
#define _UTIH32_

BOOL MakeWindowTransparent(HWND hWnd, unsigned char factor);
void SendF4();
bool GetStateButton(HWND hwnd, int controlID);
void Command(HWND hwnd, int controlID, int command);

/*
#define FOREGROUND_BLUE              1      // Text color contains blue.
#define FOREGROUND_GREEN             2      //  Text color contains green.
#define FOREGROUND_RED               4      //  Text color contains red.
#define FOREGROUND_INTENSITY         8      //  Text color is intensified.
#define BACKGROUND_BLUE             10      //  Background color contains blue.
#define BACKGROUND_GREEN            20      //  Background color contains green.
#define BACKGROUND_RED              40      //  Background color contains red.
#define BACKGROUND_INTENSITY        80      //  Background color is intensifie
*/

enum console_color {
	TXT_CYAN = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	TXT_GREEN = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	TXT_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	TXT_RED = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
};

void SetConsoleColorTXT(console_color c);

#endif
