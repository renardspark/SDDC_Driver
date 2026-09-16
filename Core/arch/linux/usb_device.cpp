/*
 * usb_device.c - Basic USB and USB control functions
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
 *  - FX3 SDK for Linux Platforms (https://www.cypress.com/documentation/software-and-drivers/ez-usb-fx3-software-development-kit)
 *    example: cyusb_linux_1.0.5/src/download_fx3.cpp
 */

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libusb.h>
#include <stdexcept>
#include <format>

#ifdef _WIN32
void usleep(__int64 usec) 
{ 
    HANDLE timer; 
    LARGE_INTEGER ft; 

    ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL); 
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
    WaitForSingleObject(timer, INFINITE); 
    CloseHandle(timer); 
}
#else
#include <unistd.h>
#endif

#include "usb_device.h"
#include "usb_device_internals.h"
#include "ezusb.h"
#include "../../config.h"

using namespace std;

const char TAG[] = "usb_device";

typedef struct usb_device usb_device_t;

/* internal functions */
static int load_image(libusb_device_handle *dev_handle,
                      const char *image);


struct usb_device_id {
  uint16_t vid;
  uint16_t pid;
  int needs_firmware;
};


static struct usb_device_id usb_device_ids[] = {
  { 0x04b4, 0x00f3, 1 },     /* Cypress / FX3 Boot-loader */
  { 0x04b4, 0x00f1, 0 },      /* Cypress / FX3 Streamer Example */
  { 0x0781, 0x5581, 0 }
};
static int n_usb_device_ids = sizeof(usb_device_ids) / sizeof(usb_device_ids[0]);


USBDevice::USBDevice()
{
  int ret = libusb_init_context(&usb_ctx, /*options=*/nullptr, /*num_options=*/0);
  if(ret < 0) {
    ErrorPrintln(TAG, "Failed to initialize USB context: %s", libusb_strerror(ret));
    throw runtime_error(format("{} ({}:{}) ", __FUNCTION__, __FILE__, __LINE__) + libusb_error_name(ret) + " " + libusb_strerror(ret));
  }

  #ifdef _DEBUG
    ret = libusb_set_option(usb_ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
    if(ret < 0) {
      ErrorPrintln(TAG, "Failed to set libusb log level: %s", libusb_strerror(ret));
      throw runtime_error(format("{} ({}:{}) ", __FUNCTION__, __FILE__, __LINE__) + libusb_error_name(ret) + " " + libusb_strerror(ret));
    }
  #endif
}
USBDevice::~USBDevice()
{
  TracePrintln(TAG, "");

  streaming_close();
  libusb_exit(usb_ctx);
}


vector<USBDeviceInfo> USBDevice::getDeviceList()
{
  TracePrintln(TAG, "");

  vector<USBDeviceInfo> device_infos;
  const int MAX_STRING_BYTES = 256;
  char temporary_string[MAX_STRING_BYTES];

  libusb_device **list;
  ssize_t list_length = libusb_get_device_list(usb_ctx, &list);
  if (list_length < 0) {
    ErrorPrintln(TAG, "Failed to get the device list: %s", libusb_strerror(list_length));
    return vector<USBDeviceInfo>{};
  }
  
  for (ssize_t dev_index = 0; dev_index < list_length; dev_index++)
  {
    libusb_device *device = list[dev_index];

    struct libusb_device_descriptor desc;
    int ret = libusb_get_device_descriptor(device, &desc);

    for (int i = 0; i < n_usb_device_ids; ++i)
    {
      if(desc.idVendor == usb_device_ids[i].vid &&
        desc.idProduct == usb_device_ids[i].pid)
      {
        USBDeviceInfo dev_info;
        dev_info.usb_vendor_id      = desc.idVendor;
        dev_info.usb_product_id     = desc.idProduct;
        dev_info.usb_bus_number     = libusb_get_bus_number(device);
        dev_info.usb_device_address = libusb_get_device_address(device);
        dev_info.need_firmware      = usb_device_ids[i].needs_firmware;

        libusb_device_handle *dev_handle = 0;
        ret = libusb_open(device, &dev_handle);
        if (ret < 0) {
          ErrorPrintln(TAG, "Failed to open device: %s", libusb_strerror(ret));
          goto FAIL2;
        }

        if (desc.iManufacturer) {
          ret = libusb_get_string_descriptor_ascii(dev_handle, desc.iManufacturer,
                        (unsigned char*)temporary_string, MAX_STRING_BYTES);
          if (ret < 0) {
            ErrorPrintln(TAG, "Failed to get manufacturer name: %s", libusb_strerror(ret));
            goto FAIL3;
          }

          dev_info.manufacturer = temporary_string;
        }

        if (desc.iProduct) {
          ret = libusb_get_string_descriptor_ascii(dev_handle, desc.iProduct,
                        (unsigned char*)temporary_string, MAX_STRING_BYTES);
          if (ret < 0) {
            ErrorPrintln(TAG, "Failed to get product name: %s", libusb_strerror(ret));
            goto FAIL3;
          }

          dev_info.product = temporary_string;
        }

        if (desc.iSerialNumber) {
          ret = libusb_get_string_descriptor_ascii(dev_handle, desc.iSerialNumber,
                        (unsigned char*)temporary_string, MAX_STRING_BYTES);
          if (ret < 0) {
            ErrorPrintln(TAG, "Failed to get serial number: %s", libusb_strerror(ret));
            goto FAIL3;
          }

          dev_info.serial_number = temporary_string;
        }
        device_infos.push_back(dev_info);
        ret = 0;
FAIL3:
        libusb_close(dev_handle);
        if (ret < 0) {
          goto FAIL2;
        }
      }
    }
  }

FAIL2:
  libusb_free_device_list(list, 1);

  return device_infos;
}


sddc_err_t USBDevice::open(USBDeviceInfo device_def, const char* image,
                              uint32_t size)
{
  libusb_device *device = findUSBDevice(device_def);
  if (device == 0) {
    ErrorPrintln(TAG, "No USB device corresponds to the given object");
    return ERR_USB_DEVICE_NOT_FOUND;
  }
  
  dev_handle = initializeUSBDevice(device);
  if (dev_handle == nullptr) {
    ErrorPrintln(TAG, "Unable to open the USB device");
    return ERR_USB_OPEN_FAILED;
  }

  if (device_def.need_firmware) {
    int ret = load_image(dev_handle, image);
    if (ret != 0) {
      ErrorPrintln(TAG, "Failed to load firmware image in the SDR");
    }

    libusb_unref_device(device);
    libusb_release_interface(dev_handle, 0);
    libusb_close(dev_handle);

    /* wait unitl firmware is ready */
    usleep(500 * 1000L);

    device = findUSBDevice(device_def, false);

    if (device == 0) {
      ErrorPrintln(TAG, "The USB device is gone");
      return ERR_USB_STUCK_IN_BOOTLOADER;
    }

    dev_handle = initializeUSBDevice(device);
    if (dev_handle == nullptr) {
      ErrorPrintln(TAG, "Unable to open the USB device");
      return ERR_USB_OPEN_FAILED;
    }

    // TODO: Check if the device really got the firmware
  }

  int speed = libusb_get_device_speed(device);
  if ( speed == LIBUSB_SPEED_LOW || speed == LIBUSB_SPEED_FULL || speed == LIBUSB_SPEED_HIGH ) {
      ErrorPrintln(TAG, "The USB device isn't capable of using USB 3.x SuperSpeed");
      libusb_unref_device(device);
      libusb_release_interface(dev_handle, 0);
      libusb_close(dev_handle);
      return ERR_USB_USB3_UNAVAILABLE;
  }

  /* list endpoints */
  struct libusb_endpoint_descriptor endpoints[MAX_ENDPOINTS];
  struct libusb_ss_endpoint_companion_descriptor ss_endpoints[MAX_ENDPOINTS];
  int ret = list_endpoints(endpoints, ss_endpoints, device);
  if (ret < 0) {
    ErrorPrintln(TAG, "Failed to list endpoints");
    libusb_unref_device(device);
    libusb_release_interface(dev_handle, 0);
    libusb_close(dev_handle);
    return ERR_USB_LIST_ENDPOINTS_FAILED;
  }

  // No need for the device pointer anymore
  libusb_unref_device(device);

  int nendpoints = ret;
  uint8_t bulk_in_endpoint_address = 0;
  uint16_t bulk_in_max_packet_size = 0;
  uint8_t bulk_in_max_burst = 0;
  for (int i = 0; i < nendpoints; ++i) {
    if ((endpoints[i].bmAttributes & 0x03) == LIBUSB_TRANSFER_TYPE_BULK &&
        (endpoints[i].bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_IN) {
      bulk_in_endpoint_address = endpoints[i].bEndpointAddress;
      bulk_in_max_packet_size = endpoints[i].wMaxPacketSize;
      bulk_in_max_burst = ss_endpoints[i].bLength == 0 ? 0 :
                          ss_endpoints[i].bMaxBurst;
      break;
    }
  }
  if (bulk_in_endpoint_address == 0) {
    ErrorPrintln(TAG, "No bulk IN endpoint found");
    libusb_release_interface(dev_handle, 0);
    libusb_close(dev_handle);
    return ERR_USB_NO_BULK_IN_ENDPOINT;
  }

  /* we are good here - create and initialize the usb_device */

  this->bulk_in_endpoint_address = bulk_in_endpoint_address;
  this->bulk_in_max_packet_size = bulk_in_max_packet_size;
  this->bulk_in_max_burst = bulk_in_max_burst;

  return ERR_SUCCESS;
}


void USBDevice::close()
{
  TracePrintln(TAG, "");

  if(dev_handle != nullptr)
  {
    libusb_release_interface(dev_handle, 0);
    libusb_close(dev_handle);
  }
}


int USBDevice::handleEvents()
{
  return libusb_handle_events_completed(usb_ctx, &completed);
}

/**
 * @brief Send a request to the USB device
 * 
 * @param[in] t usb_device handle
 * @param[in] request
 * @param[in] value
 * @param[in] index
 * @param[in] data pointer to a data buffer
 * @param[in] length length of the data buffer
 * @param[in] read Read request if true, write request otherwise
 * 
 * \retval 0
 * \retval -1
 */
int USBDevice::control(uint8_t request, uint16_t value,
                       uint16_t index, uint8_t *data, uint16_t length, bool read) {

  const uint8_t bmWriteRequestType = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
  const uint8_t bmReadRequestType = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
  const unsigned int timeout = 5000;        // timeout (in ms) for each command
  int ret;

  if (!read) {
      ret = libusb_control_transfer(dev_handle, bmWriteRequestType,
                                    request, value, index, data, length,
                                    timeout);
      if (ret < 0) {
        ErrorPrintln(TAG, "Failed to send USB command: %s", libusb_strerror(ret));
        return -1;
      }
  }
  else
  {
      ret = libusb_control_transfer(dev_handle, bmReadRequestType,
                                    request, value, index, data, length,
                                    timeout);
      // LIBUSB_ERROR_PIPE indicates that the device voluntarily closed
      // the connection, hence not an error
      if (ret < 0 && ret != LIBUSB_ERROR_PIPE) {
        ErrorPrintln(TAG, "Failed to send USB command: %s", libusb_strerror(ret));
        return -1;
      }
  }

  return 0;
}

libusb_device_handle* USBDevice::initializeUSBDevice(libusb_device *device)
{
  libusb_device_handle *dev_handle = nullptr;
  int ret = libusb_open(device, &dev_handle);
  if (ret < 0) {
    ErrorPrintln(TAG, "Failed to open device: %s", libusb_strerror(ret));
    return nullptr;
  }

#ifndef _WIN32
  ret = libusb_kernel_driver_active(dev_handle, 0);
  if (ret < 0) {
    libusb_close(dev_handle);
    ErrorPrintln(TAG, "Failed to check if a kernel driver is active: %s", libusb_strerror(ret));
    return nullptr;
  }
  if (ret == 1) {
    libusb_close(dev_handle);
    ErrorPrintln(TAG, "A kernel driver is active on the device. This prevents use by " SWNAME);
    return nullptr;
  }
#endif

  ret = libusb_claim_interface(dev_handle, 0);
  if (ret < 0) {
    libusb_close(dev_handle);
    ErrorPrintln(TAG, "Failed to claim device interface: %s", libusb_strerror(ret));
    return nullptr;
  }

  return dev_handle;
}


libusb_device* USBDevice::findUSBDevice(USBDeviceInfo device_def, bool strict)
{
  libusb_device *device = nullptr;

  libusb_device **list = 0;
  ssize_t list_length = libusb_get_device_list(usb_ctx, &list);
  if (list_length < 0) {
    ErrorPrintln(TAG, "Failed to get the device list: %s", libusb_strerror(list_length));
    return nullptr;
  }

  for (ssize_t dev_index = 0; dev_index < list_length; dev_index++)
  {
    libusb_device *dev = list[dev_index];

    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(dev, &desc);

    // Those checks are enough to make sure we target
    // the correct device (same device in the same physical port)
    if(strict &&
      desc.idVendor == device_def.usb_vendor_id &&
      desc.idProduct == device_def.usb_product_id &&
      libusb_get_bus_number(dev) == device_def.usb_bus_number &&
      libusb_get_device_address(dev) == device_def.usb_device_address)
    {
      device = dev;

      // The device is referenced to keep the ref counter
      // at the same level after the unref below
      libusb_ref_device(dev);
      break;
    }
    else if(!strict)
    {
      // Non-strict mode is useful when the device has already
      // been confirmed to be the right one, but it has updated
      // and the PID/VID changed
      for(int i = 0; i < n_usb_device_ids; ++i)
      {
        if(desc.idVendor == usb_device_ids[i].vid &&
          desc.idProduct == usb_device_ids[i].pid &&
          libusb_get_bus_number(dev) == device_def.usb_bus_number)
        {
          device = dev;

          // The device is referenced to keep the ref counter
          // at the same level after the unref below
          libusb_ref_device(dev);
          break;
        }
      }
    }
  }

  libusb_free_device_list(list, 1);

  return device;
}


int load_image(libusb_device_handle *dev_handle, const char *image)
{
  verbose = 1;
  return fx3_load_ram(dev_handle, image);
}

int USBDevice::list_endpoints(struct libusb_endpoint_descriptor endpoints[],
                          struct libusb_ss_endpoint_companion_descriptor ss_endpoints[],
                          libusb_device *device)
{
  struct libusb_config_descriptor *config;
  int ret = libusb_get_active_config_descriptor(device, &config);
  if (ret < 0) {
    ErrorPrintln(TAG, "Failed to list endpoints: %s", libusb_strerror(ret));
    return -1;
  }

  int count = 0;

  /* loop through the interfaces */
  for (int intf = 0; intf < config->bNumInterfaces; ++intf) {
    const struct libusb_interface *interface = &config->interface[intf];
    for (int setng = 0; setng < interface->num_altsetting; ++setng) {
      const struct libusb_interface_descriptor *setting = &interface->altsetting[setng];
      for (int endp = 0; endp < setting->bNumEndpoints; ++endp) {
        const struct libusb_endpoint_descriptor *endpoint = &setting->endpoint[endp];
        if (count == MAX_ENDPOINTS) {
          fprintf(stderr, "WARNING - found too many USB endpoints; returning only the first %d\n", MAX_ENDPOINTS);
          return count;
        }
        endpoints[count] = *endpoint;
        struct libusb_ss_endpoint_companion_descriptor *endpoint_ss_companion;
        ret = libusb_get_ss_endpoint_companion_descriptor(usb_ctx, endpoint,
                &endpoint_ss_companion);

        //printf("PktSize=%d\n", endpoint->wMaxPacketSize * (endpoint_ss_companion->bMaxBurst + 1));
        if (ret < 0 && ret != LIBUSB_ERROR_NOT_FOUND) {
          ErrorPrintln(TAG, "Failed to list endpoints: %s", libusb_strerror(ret));
          return -1;
        }
        if (ret == 0) {
          ss_endpoints[count] = *endpoint_ss_companion;
        } else {
          ss_endpoints[count].bLength = 0;
        }
        libusb_free_ss_endpoint_companion_descriptor(endpoint_ss_companion);
        count++;
      }
    }
  }

  libusb_free_config_descriptor(config);

  return count;
}
