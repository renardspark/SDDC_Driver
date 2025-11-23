/*
 * streaming.c - streaming functions
 *
 * Copyright (C) 2020 by Franco Venturi
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
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* References:
 *  - librtlsdr.c: https://github.com/librtlsdr/librtlsdr/blob/development/src/librtlsdr.c
 *  - Ettus Research UHD libusb1_zero_copy.cpp: https://github.com/EttusResearch/uhd/blob/master/host/lib/transport/libusb1_zero_copy.cpp
 */


#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
extern void usleep(__int64 usec);
#else
#include <unistd.h>
#endif

#include "streaming.h"
#include "usb_device.h"
#include "usb_device_internals.h"
#include "logging.h"
#include "../../config.h"

using namespace std;

typedef struct streaming streaming_t;

/* internal functions */
static void LIBUSB_CALL streaming_read_async_callback(struct libusb_transfer *transfer);


enum StreamingStatus {
  STREAMING_STATUS_OFF,
  STREAMING_STATUS_READY,
  STREAMING_STATUS_STREAMING,
  STREAMING_STATUS_CANCELLED,
  STREAMING_STATUS_FAILED = 0xff
};

typedef struct streaming {
  enum StreamingStatus status;
  int random;
  usb_device_t *usb_device;
  uint32_t sample_rate;
  uint32_t frame_size;
  uint32_t num_frames;
  streaming_read_async_cb_t callback;
  void *callback_context;
  uint8_t **frames;
  struct libusb_transfer **transfers;
  atomic_int active_transfers;
} streaming_t;


static const uint32_t DEFAULT_SAMPLE_RATE = 64000000;   /* 64Msps */
const unsigned int BULK_XFER_TIMEOUT = 5000; // timeout (in ms) for each bulk transfer


streaming_t *streaming_open_sync(usb_device_t *usb_device)
{
  streaming_t *ret_val = 0;

  /* we must have a bulk in device to transfer data from */
  if (usb_device->bulk_in_endpoint_address == 0) {
    log_error("no USB bulk in endpoint found", __func__, __FILE__, __LINE__);
    return ret_val;
  }

  /* we are good here - create and initialize the streaming */
  streaming_t *t = (streaming_t *) malloc(sizeof(streaming_t));
  t->status = STREAMING_STATUS_READY;
  t->random = 0;
  t->usb_device = usb_device;
  t->sample_rate = DEFAULT_SAMPLE_RATE;
  t->frame_size = 0;
  t->num_frames = 0;
  t->callback = 0;
  t->callback_context = 0;
  t->frames = 0;
  t->transfers = 0;
  t->active_transfers = 0;

  ret_val = t;
  return ret_val;
}

int streaming_framesize(streaming_t *that)
{
  return that->frame_size;
}

streaming_t *streaming_open_async(usb_device_t *usb_device, uint32_t frame_size,
                      uint32_t num_frames, streaming_read_async_cb_t callback,
                      void *callback_context)
{
  streaming_t *ret_val = 0;

  /* we must have a bulk in device to transfer data from */
  if (usb_device->bulk_in_endpoint_address == 0) {
    log_error("no USB bulk in endpoint found", __func__, __FILE__, __LINE__);
    return ret_val;
  }

  /* frame size must be a multiple of max_packet_size * (max_burst + 1) */
  uint32_t max_xfer_size = usb_device->bulk_in_max_packet_size *
                           (usb_device->bulk_in_max_burst + 1);
  if ( !max_xfer_size ) {
    fprintf(stderr, "ERROR: maximum transfer size is 0. probably not connected at USB 3 port?!\n");
    return ret_val;
  }

  int iso_packets_per_frame = frame_size / usb_device->bulk_in_max_packet_size;
  // fprintf(stderr, "frame_size = %u, iso_packets_per_frame = %d\n", (unsigned)frame_size, iso_packets_per_frame);

  if (frame_size % max_xfer_size != 0) {
    fprintf(stderr, "frame size must be a multiple of %d\n", max_xfer_size);
    return ret_val;
  }

  /* allocate frames for zerocopy USB bulk transfers */
  uint8_t **frames = (uint8_t **) malloc(num_frames * sizeof(uint8_t *));
  for (uint32_t i = 0; i < num_frames; ++i) {
    #ifdef __linux__
    frames[i] = libusb_dev_mem_alloc(usb_device->dev_handle, frame_size);
    #elif defined(__APPLE__)
    frames[i] = (uint8_t *) malloc(frame_size);
    #else
    frames[i] = (uint8_t *) malloc(frame_size);
    #endif

    if (frames[i] == 0) {
      log_error("libusb_dev_mem_alloc() failed", __func__, __FILE__, __LINE__);
      for (uint32_t j = 0; j < i; j++) {
        #ifdef __linux__
        libusb_dev_mem_free(usb_device->dev_handle, frames[j], frame_size);
        #elif defined(__APPLE__)
        free(frames[j]);
        #else
        free(frames[i]);
        #endif
      }
      return ret_val;
    }
  }

  /* we are good here - create and initialize the streaming */
  streaming_t *t = (streaming_t *) malloc(sizeof(streaming_t));
  t->status = STREAMING_STATUS_READY;
  t->random = 0;
  t->usb_device = usb_device;
  t->sample_rate = DEFAULT_SAMPLE_RATE;
  t->frame_size = frame_size;
  t->num_frames = num_frames;
  t->callback = callback;
  t->callback_context = callback_context;
  t->frames = frames;

  /* populate the required libusb_transfer fields */
  struct libusb_transfer **transfers = (struct libusb_transfer **) malloc(num_frames * sizeof(struct libusb_transfer *));
  for (uint32_t i = 0; i < num_frames; ++i) {
    transfers[i] = libusb_alloc_transfer(0);	// iso_packets_per_frame ?
    libusb_fill_bulk_transfer(transfers[i], usb_device->dev_handle,
                              usb_device->bulk_in_endpoint_address,
                              frames[i], frame_size, (libusb_transfer_cb_fn)streaming_read_async_callback,
                              t, BULK_XFER_TIMEOUT);
  }
  t->transfers = transfers;
  t->active_transfers = 0;

  ret_val = t;
  return ret_val;
}


void streaming_close(streaming_t *t)
{
  if (t->transfers) {
    for (uint32_t i = 0; i < t->num_frames; ++i) {
      libusb_free_transfer(t->transfers[i]);
    }
    free(t->transfers);
  }
  if (t->frames != 0) {
    for (uint32_t i = 0; i < t->num_frames; ++i) {
      #ifdef __linux__
      libusb_dev_mem_free(t->usb_device->dev_handle, t->frames[i],
                          t->frame_size);
      #elif defined(__APPLE__)
      free(t->frames[i]);
      #else
      free(t->frames[i]);
      #endif
    }
    free(t->frames);
  }
  free(t);
  return;
}


int streaming_set_sample_rate(streaming_t *t, uint32_t sample_rate)
{
  /* no checks yet */
  t->sample_rate = sample_rate;
  return 0;
}


int streaming_set_random(streaming_t *t, int random)
{
  t->random = random;
  return 0;
}


int streaming_start(streaming_t *t)
{
  if (t->status != STREAMING_STATUS_READY) {
    fprintf(stderr, "ERROR - streaming_start() called with streaming status not READY: %d\n", t->status);
    return -1;
  }

  /* if there is no callback, then streaming is synchronous - nothing to do */
  if (t->callback == 0) {
    t->status = STREAMING_STATUS_STREAMING;
    return 0;
  }

  /* submit all the transfers */
  t->active_transfers = 0;
  for (uint32_t i = 0; i < t->num_frames; ++i) {
    int ret = libusb_submit_transfer(t->transfers[i]);
    if (ret < 0) {
      log_usb_error(ret, __func__, __FILE__, __LINE__);
      t->status = STREAMING_STATUS_FAILED;
      return -1;
    }
    t->active_transfers.fetch_add(1);
  }

  t->status = STREAMING_STATUS_STREAMING;

  return 0;
}


int streaming_stop(streaming_t *t)
{
  /* if there is no callback, then streaming is synchronous - nothing to do */
  if (t->callback == 0) {
    if (t->status == STREAMING_STATUS_STREAMING) {
      t->status = STREAMING_STATUS_READY;
    }
    return 0;
  }

  t->status = STREAMING_STATUS_CANCELLED;

  /* flush all the events */
  struct timeval noblock = { 0, 0 };
  while (t->active_transfers > 0) {
    int ret = libusb_handle_events_timeout_completed(t->usb_device->context, &noblock, 0);
    if (ret < 0) {
      log_usb_error(ret, __func__, __FILE__, __LINE__);
      t->status = STREAMING_STATUS_FAILED;
    }
    usleep(100);
  }

  /* cancel all the active transfers */
  for (uint32_t i = 0; i < t->num_frames; ++i) {
    int ret = libusb_cancel_transfer(t->transfers[i]);
    if (ret < 0) {
      if (ret == LIBUSB_ERROR_NOT_FOUND)  {
        continue;
      }
      log_usb_error(ret, __func__, __FILE__, __LINE__);
      t->status = STREAMING_STATUS_FAILED;
    }
  }

  
  return 0;
}


int streaming_reset_status(streaming_t *t)
{
  switch (t->status) {
    case STREAMING_STATUS_READY:
      /* nothing to do here */
      return 0;
    case STREAMING_STATUS_CANCELLED:
    case STREAMING_STATUS_FAILED:
      {
        int active_transfers = t->active_transfers;
        if (active_transfers > 0) {
          fprintf(stderr, "ERROR - streaming_reset_status() called with %d transfers still active\n",
                          active_transfers);
          return -1;
        }
        break;
      }
    default:
      fprintf(stderr, "ERROR - streaming_reset_status() called with invalid status: %d\n",
                      t->status);
      return -1;
  }

  /* we are good here; reset the status */
  t->status = STREAMING_STATUS_READY;
  return 0;
}


int streaming_read_sync(streaming_t *t, uint8_t *data, int length, int *transferred)
{
  int ret = libusb_bulk_transfer(t->usb_device->dev_handle,
                                 t->usb_device->bulk_in_endpoint_address,
                                 data, length, transferred, BULK_XFER_TIMEOUT);
  if (ret < 0) {
    log_usb_error(ret, __func__, __FILE__, __LINE__);
    return -1;
  }

  /* remove ADC randomization */
  if (t->random) {
    uint16_t *samples = (uint16_t *) data;
    int n = *transferred / 2;
    for (int i = 0; i < n; ++i) {
      if (samples[i] & 1) {
        samples[i] ^= 0xfffe;
      }
    }
  }

  return 0;
}


/* internal functions */
static void LIBUSB_CALL streaming_read_async_callback(struct libusb_transfer *transfer)
{
  streaming_t *t = (streaming_t *) transfer->user_data;
  int ret;
  switch (transfer->status) {
    case LIBUSB_TRANSFER_COMPLETED:
      /* success!!! */
      if (t->status == STREAMING_STATUS_STREAMING) {
        /* remove ADC randomization */
        if (t->random) {
          uint16_t *samples = (uint16_t *) transfer->buffer;
          int n = transfer->actual_length / 2;
          for (int i = 0; i < n; ++i) {
            if (samples[i] & 1) {
              samples[i] ^= 0xfffe;
            }
          }
        }
        t->callback(transfer->actual_length, transfer->buffer,
                       t->callback_context);
        ret = libusb_submit_transfer(transfer);
        if (ret == 0) {
          return;
        }
        log_usb_error(ret, __func__, __FILE__, __LINE__);
      }
      break;
    case LIBUSB_TRANSFER_CANCELLED:
      /* librtlsdr does also ignore LIBUSB_TRANSFER_CANCELLED */
      t->active_transfers.fetch_sub(1);
      return;
    case LIBUSB_TRANSFER_ERROR:
    case LIBUSB_TRANSFER_TIMED_OUT:
    case LIBUSB_TRANSFER_STALL:
    case LIBUSB_TRANSFER_NO_DEVICE:
    case LIBUSB_TRANSFER_OVERFLOW:
      log_usb_error(transfer->status, __func__, __FILE__, __LINE__);
      break;
  }

  t->status = STREAMING_STATUS_FAILED;
  t->active_transfers.fetch_sub(1);

  /* cancel all the active transfers */
  for (uint32_t i = 0; i < t->num_frames; ++i) {
    int ret = libusb_cancel_transfer(transfer);
    if (ret < 0) {
      if (ret == LIBUSB_ERROR_NOT_FOUND) {
        continue;
      }
      log_usb_error(ret, __func__, __FILE__, __LINE__);
    }
  }
  return;
}