/*
 * This file is part of SDDC_Driver.
 *
 * =====================
 *      MIT License
 * =====================
 *
 * Copyright (C) 2025 - RenardSpark
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

#ifndef _H_TYPES
#define _H_TYPES

#include <stdint.h>

/**
 * @brief List of all the errors which may be returned
 * 
 */
typedef enum sddc_err_t {
	ERR_SUCCESS = 0x00, ///< No error
	ERR_FX3_OPEN_FAILED,
	ERR_FX3_DEVICE_BUSY,
	ERR_FX3_TRANSFER_FAILED,
	ERR_NOT_COMPATIBLE = -0x10, ///< The function is not compatible with the current hardware
	ERR_DECIMATION_OUT_OF_RANGE, ///< The given decimation is out of the allowed range
	ERR_NOT_LED, ///< The selected LED is not an LED
	ERR_BUFFER_SIZE_INVALID,
	ERR_OUT_OF_RANGE,
	ERR_USB_DEVICE_NOT_FOUND,
	ERR_USB_STUCK_IN_BOOTLOADER,
	ERR_USB_USB3_UNAVAILABLE,
	ERR_USB_LIST_ENDPOINTS_FAILED,
	ERR_USB_OPEN_FAILED,
	ERR_USB_NO_BULK_IN_ENDPOINT
} sddc_err_t;

typedef enum sddc_rf_mode_t {
	NOMODE  = 0x00,
	HFMODE  = 0x01,
	VHFMODE = 0x02
} sddc_rf_mode_t;

typedef enum sddc_leds_t {
	SDDC_LED_YELLOW = 0x01,
	SDDC_LED_RED    = 0x02,
	SDDC_LED_BLUE   = 0x04
} sddc_leds_t;

typedef float sddc_complex_t[2];

#endif // _H_TYPES