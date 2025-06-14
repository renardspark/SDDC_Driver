#ifndef FX3CLASS_H
#define FX3CLASS_H

//
// FX3handler.cpp 
// 2020 10 12  Oscar Steila ik1xpv
// loading arm code.img from resource by Howard Su and Hayati Ayguen
// This module was previous named:openFX3.cpp
// MIT License Copyright (c) 2016 Booya Corp.
// booyasdr@gmail.com, http://booyasdr.sf.net
// modified 2017 11 30 ik1xpv@gmail.com, http://www.steila.com/blog
// 

#include <stdint.h>
#include <functional>

#include "config.h"
#include "../Interface.h"
#include "dsp/ringbuffer.h"

using namespace std;

class fx3class
{
public:
	virtual ~fx3class(void) {}
	virtual bool Open(uint8_t dev_index) = 0;
	virtual bool Control(FX3Command command, uint8_t data = 0) = 0;
	virtual bool Control(FX3Command command, uint32_t data) = 0;
	virtual bool Control(FX3Command command, uint64_t data) = 0;
	virtual bool SetArgument(uint16_t index, uint16_t value) = 0;
	virtual bool GetHardwareInfo(uint32_t* data) = 0;
	virtual bool ReadDebugTrace(uint8_t* pdata, uint8_t len) = 0;
	virtual void StartStream(ringbuffer<int16_t>& input) = 0;
	virtual void StopStream() = 0;
	virtual bool Enumerate(unsigned char& idx, char* lbuf) = 0;
	virtual size_t GetDeviceListLength() = 0;
	virtual bool GetDevice(unsigned char &idx, char *name, size_t name_len, char *serial, size_t serial_len) = 0;
	virtual vector<SDDC::DeviceItem> GetDeviceList() = 0;
};

extern "C" fx3class* CreateUsbHandler();

#endif // FX3CLASS_H
