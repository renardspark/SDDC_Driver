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

#ifndef __USB_DEVICE_H
#define __USB_DEVICE_H

#include <libusb.h>
#include <vector>
#include <string>

typedef struct USBDeviceInfo {
  uint8_t index;
  uint16_t usb_vendor_id;
  uint16_t usb_product_id;
  bool need_firmware;
  std::string manufacturer;
  std::string product;
  std::string serial_number;
} USBDeviceInfo;


typedef struct usb_device usb_device_t;

void usb_device_init();
void usb_device_destroy();


std::vector<USBDeviceInfo> usb_device_get_device_list();

usb_device_t *usb_device_open(USBDeviceInfo dev_select, const char* image,
                              uint32_t size);

int usb_device_handle_events(usb_device_t *t);

void usb_device_close(usb_device_t *t);

int usb_device_control(usb_device_t *t, uint8_t request, uint16_t value,
                       uint16_t index, uint8_t *data, uint16_t length, bool read);

#endif /* __USB_DEVICE_H */
