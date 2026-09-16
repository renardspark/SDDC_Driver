/*
 * usb_device.h - Basic USB and USB control functions
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

#pragma once

#include "../config.h"
#include <libusb.h>
#include <vector>
#include <string>

typedef struct USBDeviceInfo {
  // uint8_t index;
  uint16_t usb_vendor_id;
  uint16_t usb_product_id;
  bool need_firmware;
  std::string manufacturer;
  std::string product;
  std::string serial_number;
  uint8_t usb_bus_number;
  uint8_t usb_device_address;
} USBDeviceInfo;

enum StreamingStatus {
  STREAMING_STATUS_OFF,
  STREAMING_STATUS_READY,
  STREAMING_STATUS_STREAMING,
  STREAMING_STATUS_CANCELLED,
  STREAMING_STATUS_FAILED = 0xff
};

typedef struct streaming streaming_t;

typedef void (*streaming_read_async_cb_t)(uint32_t data_size, uint8_t *data,
                                          void *context);

#define MAX_ENDPOINTS (16)
#define BULK_XFER_TIMEOUT 5000 // timeout (in ms) for each bulk transfer


class USBDevice
{
  public:
    USBDevice();
    ~USBDevice();

    std::vector<USBDeviceInfo> getDeviceList();

    sddc_err_t open(USBDeviceInfo dev_select, const char* image, uint32_t size);
    void close();
    int control(uint8_t request, uint16_t value, uint16_t index, uint8_t *data, uint16_t length, bool read);
    int handleEvents();

    int streaming_open_sync();
    int streaming_open_async(uint32_t frame_size,
                      uint32_t num_frames, streaming_read_async_cb_t callback,
                      void *callback_context);
    int streaming_framesize();
    void streaming_close();
    int streaming_start();
    int streaming_stop();
    int streaming_reset_status();
    int streaming_read_sync(uint8_t *data, int length,
                            int *transferred);

  private:
    // --- USB --- //
    libusb_context *usb_ctx = nullptr;
    libusb_device_handle *dev_handle = nullptr;

    libusb_device* findUSBDevice(USBDeviceInfo device_def, bool strict = true);
    libusb_device_handle* initializeUSBDevice(libusb_device *device);
    // --- //


    uint8_t bulk_in_endpoint_address = 0;
    uint16_t bulk_in_max_packet_size = 0;
    uint8_t bulk_in_max_burst = 0;

    int list_endpoints(struct libusb_endpoint_descriptor endpoints[],
      struct libusb_ss_endpoint_companion_descriptor ss_endpoints[],
      libusb_device *device);

    // --- Streaming --- //
    uint32_t concurrent_transfers;
    std::vector<struct libusb_transfer*> transfers;
    std::vector<uint8_t*> transfer_buffers;
    streaming_t *streaming_obj = nullptr;

    StreamingStatus streaming_status;

    streaming_read_async_cb_t stream_callback;
    void *stream_callback_context;

    static void LIBUSB_CALL streaming_read_async_callback(struct libusb_transfer *transfer);
};