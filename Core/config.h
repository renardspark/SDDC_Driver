/*
 * This file is part of SDDC_Driver.
 *
 * Copyright (C) 2020 - Oscar Steila
 * Copyright (C) 2020 - Howard Su
 * Copyright (C) 2021 - Hayati Ayguen
 * Copyright (C) 2025 - RenardSpark
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "types.h"
#include "types_cpp.h"
#include "../Interface.h"
#include <math.h>      // atan => PI
#include <stdbool.h>
#include <cstdio>

//#define _DEBUG  // defined in VS configuration
#define VERBOSE_ERROR
#define VERBOSE_WARN
//#define VERBOSE_TRACE
//#define VERBOSE_TRACEEXTREME

// macro to call callback function with just status extHWstatusT
#define EXTIO_STATUS_CHANGE(TAG, CB, STATUS)   \
	do { \
	  SendMessage(h_dialog, WM_USER + 1, STATUS, 0); \
	  if (CB) { \
		  DebugPrintln(TAG, "<==CALLBACK: %s", #STATUS); \
		  CB( -1, STATUS, 0, NULL );\
	  }\
	}while(0)

#ifdef VERBOSE_DEBUG
	#define EnterFunction() \
	DbgPrintf("==>%s\n", __FUNCDNAME__)

	#define EnterFunction1(v1) \
	DbgPrintf("==>%s(%d)\n", __FUNCDNAME__, (v1))
#else
	#define EnterFunction()
	#define EnterFunction1(v1)
#endif

#ifdef VERBOSE_ERROR
	#define ErrorPrint(tag, fmt, ...)   fprintf(stderr, "[SDDC] ERROR - %s: %s (%s:%d) " fmt,      tag, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
	#define ErrorPrintln(tag, fmt, ...) fprintf(stderr, "[SDDC] ERROR - %s: " fmt " (%s:%d)\n", tag, ##__VA_ARGS__, __FILE__, __LINE__)
#else
	#define ErrorPrint(tag, fmt, ...)
	#define ErrorPrintln(tag, fmt, ...)
#endif

#ifdef VERBOSE_WARN
	#define WarnPrint(tag, fmt, ...)    fprintf(stderr, "[SDDC] WARN  - %s: " fmt,      tag, ##__VA_ARGS__)
	#define WarnPrintln(tag, fmt, ...)  fprintf(stderr, "[SDDC] WARN  - %s: " fmt "\n", tag, ##__VA_ARGS__)
#else
	#define WarnPrint(tag, fmt, ...)
	#define WarnPrintln(tag, fmt, ...)
#endif

#ifdef _DEBUG
	#define DebugPrint(tag, fmt, ...)   fprintf(stderr, "[SDDC] DEBUG - %s: " fmt,      tag, ##__VA_ARGS__)
	#define DebugPrintln(tag, fmt, ...) fprintf(stderr, "[SDDC] DEBUG - %s: " fmt "\n", tag, ##__VA_ARGS__)
#else
	#define DebugPrint(tag, fmt, ...)
	#define DebugPrintln(tag, fmt, ...)
#endif

#ifdef VERBOSE_TRACE
	#define TracePrint(tag, fmt, ...)   fprintf(stderr, "[SDDC] TRACE - %s: %d-%s(" fmt ")",   tag, __LINE__, __FUNCTION__, ##__VA_ARGS__)
	#define TracePrintln(tag, fmt, ...) fprintf(stderr, "[SDDC] TRACE - %s: %d-%s(" fmt ")\n", tag, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
	#define TracePrint(tag, fmt, ...)
	#define TracePrintln(tag, fmt, ...)
#endif

#ifdef VERBOSE_TRACEEXTREME
	#define TraceExtremePrint(tag, fmt, ...)   TracePrint(tag, fmt, ##__VA_ARGS__)
	#define TraceExtremePrintln(tag, fmt, ...) TracePrintln(tag, fmt, ##__VA_ARGS__)
#else
	#define TraceExtremePrint(tag, fmt, ...)
	#define TraceExtremePrintln(tag, fmt, ...)
#endif

#define SWVERSION           "1.0.0"
#define SWNAME				"SDDC_Driver"


#define FFTN_R_ADC (8192)       // FFTN used for ADC real stream DDC  tested at  2048, 8192, 32768, 131072

// GAINFACTORS to be adjusted with lab reference source measured with HDSDR Smeter rms mode  
#define BBRF103_GAINFACTOR 	(7.8e-8f)       // BBRF103
#define HF103_GAINFACTOR   	(1.14e-8f)      // HF103
#define RX888_GAINFACTOR   	(0.695e-8f)     // RX888
#define RX888mk2_GAINFACTOR (1.08e-8f)      // RX888mk2



#define EXT_BLOCKLEN		512	* 64	/* 32768 only multiples of 512 */

// URL definitions
#define URL1B               "16bit SDR Receiver"
#define URL1                "<a>http://www.hdsdr.de/</a>"
#define URL_HDSR            "http://www.hdsdr.de/"
#define URL_HDSDRA          "<a>http://www.hdsdr.de/</a>"


extern bool saveADCsamplesflag;

// transferSize must be a multiple of 16 (maxBurst) * 1024 (SS packet size) = 16384
const uint32_t transferSize = 131072;
const uint32_t transferSamples = transferSize / sizeof(int16_t);
const uint32_t concurrentTransfers = 16;  // used to be 96, but I think it is too high

const uint32_t DEFAULT_ADC_FREQ = 64000000;	// ADC sampling frequency

const uint32_t DEFAULT_TRANSFERS_PER_SEC = DEFAULT_ADC_FREQ / transferSamples;



#define MIN_ADC_FREQ 50000000	   // ADC sampling frequency minimum
#define MAX_ADC_FREQ 140000000	// ADC sampling frequency minimum
#define N2_BANDSWITCH 80000000    // threshold 5 or 6 SR bandwidths


#endif // _CONFIG_H_
