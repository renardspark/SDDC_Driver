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

#include "usb_device.h"
#include "../../config.h"

using namespace std;

const char TAG[] = "streaming";
typedef struct streaming streaming_t;

/* internal functions */
static void LIBUSB_CALL streaming_read_async_callback(struct libusb_transfer *transfer);


typedef struct streaming {
  uint32_t frame_size;
  atomic_int active_transfers;
} streaming_t;


int USBDevice::streaming_open_sync()
{
  /* we must have a bulk in device to transfer data from */
  if (bulk_in_endpoint_address == 0) {
    ErrorPrintln(TAG, "No USB Bulk IN endpoint found");
    return -1;
  }

  streaming_status = STREAMING_STATUS_READY;
  stream_callback = nullptr;
  stream_callback_context = nullptr;

  return 0;
}

int USBDevice::streaming_open_async(uint32_t frame_size,
                      uint32_t num_frames, streaming_read_async_cb_t callback,
                      void *callback_context)
{
  TracePrintln(TAG, "%d, %d, %p, %p", frame_size, num_frames, callback, callback_context);

  /* we must have a bulk in device to transfer data from */
  if (bulk_in_endpoint_address == 0) {
    ErrorPrintln(TAG, "No bulk IN endpoint found");
    return -1;
  }

  /* frame size must be a multiple of max_packet_size * (max_burst + 1) */
  uint32_t max_xfer_size = bulk_in_max_packet_size *
                           (bulk_in_max_burst + 1);
  if ( !max_xfer_size ) {
    fprintf(stderr, "ERROR: maximum transfer size is 0. probably not connected at USB 3 port?!\n");
    return -1;
  }

  if (frame_size % max_xfer_size != 0) {
    fprintf(stderr, "frame size must be a multiple of %d\n", max_xfer_size);
    return -1;
  }

  /* allocate frames for zerocopy USB bulk transfers */
  for (uint32_t i = 0; i < num_frames; ++i) {
    #ifdef __linux__
    transfer_buffers[i] = libusb_dev_mem_alloc(dev_handle, frame_size);
    #elif defined(__APPLE__)
    transfer_buffers[i] = (uint8_t *) malloc(frame_size);
    #else
    transfer_buffers[i] = (uint8_t *) malloc(frame_size);
    #endif

    if (transfer_buffers[i] == 0) {
      ErrorPrintln(TAG, "Failed to allocate streaming buffer");
      for (uint32_t j = 0; j < i; j++) {
        #ifdef __linux__
        libusb_dev_mem_free(dev_handle, transfer_buffers[j], frame_size);
        #elif defined(__APPLE__)
        free(transfer_buffers[j]);
        #else
        free(transfer_buffers[i]);
        #endif
      }
      return -1;
    }
  }

  /* we are good here - create and initialize the streaming */
  streaming_t *t = (streaming_t *) malloc(sizeof(streaming_t));
  streaming_status = STREAMING_STATUS_READY;
  t->frame_size = frame_size;
  concurrent_transfers = num_frames;
  stream_callback = callback;
  stream_callback_context = callback_context;

  /* populate the required libusb_transfer fields */
  for (uint32_t i = 0; i < num_frames; ++i) {
    transfers[i] = libusb_alloc_transfer(0);
    libusb_fill_bulk_transfer(
      transfers[i],
      dev_handle,
      /*endpoint=*/bulk_in_endpoint_address,
      /*buffer=*/transfer_buffers[i],
      /*length=*/frame_size,
      /*callback=*/(libusb_transfer_cb_fn)streaming_read_async_callback,
      /*user_data=*/this,
      /*timeout=*/BULK_XFER_TIMEOUT
    );
  }
  t->active_transfers = 0;

  streaming_obj = t;
  return 0;
}


void USBDevice::streaming_close()
{
  TracePrintln(TAG, "");

  if(!streaming_obj) return;

  for(auto it = transfers.begin(); it != transfers.end();)
  {
    libusb_free_transfer(*it);
    it = transfers.erase(it);
  }

  for(auto it = transfer_buffers.begin(); it != transfer_buffers.end();)
  {
    #ifdef __linux__
    libusb_dev_mem_free(dev_handle, *it,
                        streaming_obj->frame_size);
    #elif defined(__APPLE__)
    free(*it);
    #else
    free(*it);
    #endif

    it = transfer_buffers.erase(it);
  }

  free(streaming_obj);
  streaming_obj = nullptr;
  return;
}


int USBDevice::streaming_start()
{
  if (streaming_status != STREAMING_STATUS_READY) {
    fprintf(stderr, "ERROR - streaming_start() called with streaming status not READY: %d\n", streaming_status);
    return -1;
  }

  /* if there is no callback, then streaming is synchronous - nothing to do */
  if (stream_callback == nullptr) {
    streaming_status = STREAMING_STATUS_STREAMING;
    return 0;
  }

  /* submit all the transfers */
  streaming_obj->active_transfers = 0;
  for(auto it: transfers) {
    int ret = libusb_submit_transfer(it);
    if (ret < 0) {
      ErrorPrintln(TAG, "Failed to submit transfer: %s", libusb_strerror(ret));
      streaming_status = STREAMING_STATUS_FAILED;
      return -1;
    }
    streaming_obj->active_transfers.fetch_add(1);
  }

  streaming_status = STREAMING_STATUS_STREAMING;

  return 0;
}


int USBDevice::streaming_stop()
{
  /* if there is no callback, then streaming is synchronous - nothing to do */
  if (stream_callback == nullptr) {
    if (streaming_status == STREAMING_STATUS_STREAMING) {
      streaming_status = STREAMING_STATUS_READY;
    }
    return 0;
  }

  streaming_status = STREAMING_STATUS_CANCELLED;

  /* flush all the events */
  struct timeval noblock = { 0, 0 };
  while (streaming_obj->active_transfers > 0) {
    int ret = libusb_handle_events_timeout_completed(usb_ctx, &noblock, 0);
    if (ret < 0) {
      ErrorPrintln(TAG, "Failed to handle events: %s", libusb_strerror(ret));
      streaming_status = STREAMING_STATUS_FAILED;
    }
    usleep(100);
  }

  /* cancel all the active transfers */
  for(auto it: transfers) {
    int ret = libusb_cancel_transfer(it);
    if (ret < 0) {
      if (ret == LIBUSB_ERROR_NOT_FOUND)  {
        continue;
      }
      ErrorPrintln(TAG, "Failed to cancel transfer: %s", libusb_strerror(ret));
      streaming_status = STREAMING_STATUS_FAILED;
    }
  }

  return 0;
}


int USBDevice::streaming_reset_status()
{
  switch (streaming_status) {
    case STREAMING_STATUS_READY:
      /* nothing to do here */
      return 0;
    case STREAMING_STATUS_CANCELLED:
    case STREAMING_STATUS_FAILED:
      {
        int active_transfers = streaming_obj->active_transfers;
        if (active_transfers > 0) {
          fprintf(stderr, "ERROR - streaming_reset_status() called with %d transfers still active\n",
                          active_transfers);
          return -1;
        }
        break;
      }
    default:
      fprintf(stderr, "ERROR - streaming_reset_status() called with invalid status: %d\n",
                      streaming_status);
      return -1;
  }

  /* we are good here; reset the status */
  streaming_status = STREAMING_STATUS_READY;
  return 0;
}


int USBDevice::streaming_read_sync(uint8_t *data, int length, int *transferred)
{
  int ret = libusb_bulk_transfer(
    dev_handle,
    bulk_in_endpoint_address,
    data,
    length,
    transferred,
    /*timeout=*/BULK_XFER_TIMEOUT
  );
  if (ret < 0) {
    ErrorPrintln(TAG, "Failed to initiate bulk transfer: %s", libusb_strerror(ret));
    return -1;
  }

  return 0;
}


/* internal functions */
void LIBUSB_CALL USBDevice::streaming_read_async_callback(struct libusb_transfer *transfer)
{
  USBDevice *t = (USBDevice*)transfer->user_data;
  int ret;
  switch (transfer->status) {
    case LIBUSB_TRANSFER_COMPLETED:
      /* success!!! */
      if (t->streaming_status == STREAMING_STATUS_STREAMING) {
        t->stream_callback(transfer->actual_length, transfer->buffer,
                       t->stream_callback_context);
        ret = libusb_submit_transfer(transfer);
        if (ret == 0) {
          return;
        }
        ErrorPrintln(TAG, "Failed to submit transfer: %s", libusb_strerror(ret));
      }
      break;
    case LIBUSB_TRANSFER_CANCELLED:
      /* librtlsdr does also ignore LIBUSB_TRANSFER_CANCELLED */
      t->streaming_obj->active_transfers.fetch_sub(1);
      return;
    case LIBUSB_TRANSFER_TIMED_OUT:
      // Time out error isn't necessarily bad if the SDR is configured on a slow sample rate
      // FIXME: This isn't perfect as the number of transfer will not increase if a faster sample rate is requested afterwards
      WarnPrintln(TAG, "Transfer timed out: %s", libusb_strerror(transfer->status));
      t->streaming_obj->active_transfers.fetch_sub(1);
      return;
    case LIBUSB_TRANSFER_ERROR:
    case LIBUSB_TRANSFER_STALL:
    case LIBUSB_TRANSFER_NO_DEVICE:
    case LIBUSB_TRANSFER_OVERFLOW:
      ErrorPrintln(TAG, "%s", libusb_strerror(transfer->status));
      break;
  }

  t->streaming_status = STREAMING_STATUS_FAILED;
  t->streaming_obj->active_transfers.fetch_sub(1);

  /* cancel all the active transfers */
  for(auto it: t->transfers)
  {
    int ret = libusb_cancel_transfer(it);
    if (ret < 0) {
      if (ret == LIBUSB_ERROR_NOT_FOUND) {
        continue;
      }
      ErrorPrintln(TAG, "Failed to cancel transfer: %s", libusb_strerror(ret));
    }
  }
}