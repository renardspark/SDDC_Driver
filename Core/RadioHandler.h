/*
 * This file is part of SDDC_Driver.
 *
 * =====================
 *      MIT License
 * =====================
 *
 * Copyright (C) 2020 - Oscar Steila
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

#ifndef _H_RADIOHANDLER
#define _H_RADIOHANDLER

#include <array>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "config.h"
#include "FX3Class.h"
#include "radio/RadioHardware.h"
#include "fft_mt_r2iq/fft_mt_r2iq.h"
#include "dsp/ringbuffer.h"

class RadioHardware;

struct shift_limited_unroll_C_sse_data_s;
typedef struct shift_limited_unroll_C_sse_data_s shift_limited_unroll_C_sse_data_t;

class RadioHandler {
public:
	RadioHandler();
	~RadioHandler();
	sddc_err_t Init(SDDC::DeviceItem dev_index);
	sddc_err_t AttachReal(void (*callback)(void* context, const int16_t*, uint32_t), void* context = nullptr);
	sddc_err_t AttachIQ(void (*callback)(void* context, const sddc_complex_t*, uint32_t), void* context = nullptr);
	sddc_err_t Start(bool convert_r2iq);
	sddc_err_t Stop();
	sddc_err_t Pause();
	sddc_err_t Resume();

	// --- r2iq --- //
	sddc_err_t	SetDecimation(uint8_t decimate);

	// ----- RF mode ----- //
	sddc_rf_mode_t	GetBestRFMode(uint64_t freq);
	sddc_rf_mode_t	GetRFMode();
	sddc_err_t		SetRFMode(sddc_rf_mode_t mode);

	// --- ADC --- //
	std::array<float, 2> GetADCSampleRateLimits();
	uint32_t	    GetADCSampleRate();
	sddc_err_t	    SetADCSampleRate(uint32_t samplefreq);

	// --- Tuner --- //
	uint32_t        GetCenterFrequency();
    sddc_err_t      SetCenterFrequency(uint32_t freq);

	// --- RF/IF adjustments --- //
	std::vector<float>   GetRFGainSteps(sddc_rf_mode_t mode = NOMODE);
	std::array<float, 2> GetRFGainRange(sddc_rf_mode_t mode = NOMODE);
	float           GetRFGain();
	sddc_err_t      SetRFGain(float new_att);

	std::vector<float>   GetIFGainSteps(sddc_rf_mode_t mode = NOMODE);
	std::array<float, 2> GetIFGainRange(sddc_rf_mode_t mode = NOMODE);
	float           GetIFGain();
	sddc_err_t      SetIFGain(float new_gain);

	// --- Misc --- //
	bool		GetBiasT_HF ();
	sddc_err_t	SetBiasT_HF (bool new_state);
	bool		GetBiasT_VHF();
	sddc_err_t	SetBiasT_VHF(bool new_state);
	bool		GetDither();
	sddc_err_t	SetDither(bool new_state);
	bool		GetPGA();
	sddc_err_t	SetPGA(bool new_state);
	bool		GetRand();
	sddc_err_t	SetRand(bool new_state);

	// --- GPIOs --- //
	sddc_err_t	SetLED (sddc_leds_t led, bool on);
	// ----- //

	float getRealSamplesPerSecond() const { return real_samples_per_second; }
	float getIQSamplesPerSecond()   const { return iq_samples_per_second; }

	/// --- Hardware infos --- //
	RadioModel getHardwareModel() { return devModel; }
	const char *getHardwareName() { return hardware->GetName(); }
	uint16_t GetHardwareFirmware() { return devFirmware; }
	// --- //


	void EnableDebug(void (*dbgprintFX3)(const char* fmt, ...), bool (*getconsolein)(char* buf, int maxlen)) 
		{ 
		  this->DbgPrintFX3 = dbgprintFX3; 
		  this->GetConsoleIn = getconsolein;
		};

	bool ReadDebugTrace(uint8_t* pdata, uint8_t len) { return fx3->ReadDebugTrace(pdata, len); }

	// --- Static functions --- //
	static size_t GetDeviceListLength();
	static std::vector<SDDC::DeviceItem> GetDeviceList();

protected:
	fx3class *fx3;

private:
	void CaculateStats();
	void OnDataPacket();

	void (*callbackReal)(void* context, const int16_t *data, uint32_t length);
	void *callbackRealContext;
	void (*callbackIQ)(void* context, const sddc_complex_t *data, uint32_t length);
	void *callbackIQContext;

	void (*DbgPrintFX3)(const char* fmt, ...);
	bool (*GetConsoleIn)(char* buf, int maxlen);

	bool streamRunning = false;

	RadioModel devModel;
	uint16_t devFirmware;

	// transfer variables
	ringbuffer<int16_t> real_buffer;
	ringbuffer<float> iq_buffer;

	// threads
	std::thread show_stats_thread;
	std::thread submit_thread;

	// --- Stats --- //
	uint32_t count_real_samples   = 0;
	uint32_t count_iq_samples     = 0;
	float real_samples_per_second = 0;
	float iq_samples_per_second   = 0;

	RadioHardware* hardware;
	std::mutex fc_mutex;
    float fc;
	shift_limited_unroll_C_sse_data_t* stateFineTune;
	fft_mt_r2iq* r2iqCntrl;
	bool r2iqEnabled = false;
};

#endif