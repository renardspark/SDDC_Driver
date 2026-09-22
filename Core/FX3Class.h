/*
 * This file is part of SDDC_Driver.
 *
 * =====================
 *      MIT License
 * =====================
 *
 * Copyright (C) 2017 - Oscar Steila
 * Copyright (C) 2020 - Howard Su
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

#ifndef FX3CLASS_H
#define FX3CLASS_H

#include <stdint.h>
#include <functional>

#include "config.h"
#include "../Interface.h"
#include "dsp/ringbuffer.h"
#include "usb/usb_device.h"


namespace SDDC {
	typedef struct DeviceItem {
		std::string product; ///< The model of the SDR
		std::string serial_number; ///< The serial number of the SDR
		USBDeviceInfo _usb_def;
	} DeviceItem;
}

class fx3class
{
public:
	virtual ~fx3class(void) {}
	virtual bool Open(SDDC::DeviceItem) = 0;
	virtual bool Control(FX3Command command, uint8_t data = 0) = 0;
	virtual bool Control(FX3Command command, uint32_t data) = 0;
	virtual bool Control(FX3Command command, uint64_t data) = 0;
	virtual bool SetArgument(uint16_t index, uint16_t value) = 0;
	virtual bool GetHardwareInfo(uint32_t* data) = 0;
	virtual bool ReadDebugTrace(uint8_t* pdata, uint8_t len) = 0;
	virtual void StartStream(ringbuffer<int16_t>& input) = 0;
	virtual void StopStream() = 0;
	virtual size_t GetDeviceListLength() = 0;
	virtual std::vector<SDDC::DeviceItem> GetDeviceList() = 0;
};

extern "C" fx3class* CreateUsbHandler();

#endif // FX3CLASS_H
