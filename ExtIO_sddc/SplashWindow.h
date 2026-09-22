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

#ifndef SPLASHWINDOW_H
#define SPLASHWINDOW_H

#include "framework.h"

//
// some idea and code from http://freesourcecode.net/cprojects/91599/Win32-splash-screen-in-c#.WmMV3KjiaUk
// downgrade and modification by ik1xpv 2018

class SplashWindow
{
public:
	SplashWindow ();
	~SplashWindow ();
	//Shows the splash screen forever till destroySplashWindow ()
    void showWindow ();
	bool createSplashWindow (HINSTANCE hinst, LPCSTR bitmapFileName, const int r, const int g, const int b);
	bool createSplashWindow (HINSTANCE hinst, DWORD bitmapResourceID, const int r, const int g, const int b);
	void destroySplashWindow ();

private:
 	bool isValidWindow () const;
	bool createWindowHelper (const int r, const int g, const int b);
	//sets all data members to 0.
	void clearMembers ();
	//window handle
	HWND mHWND;
	//Splash Window image information
	HBITMAP  mBitmap;
	int mBitmapWidth;
	int mBitmapHeight;

};


#endif
