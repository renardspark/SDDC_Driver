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
#include "logging.h"
#include "../../config.h"

using namespace std;

const char TAG[] = "usb_device";

typedef struct usb_device usb_device_t;

/* internal functions */
static libusb_device_handle *find_usb_device(USBDeviceInfo,
                             libusb_device **device, int *needs_firmware);
static int load_image(libusb_device_handle *dev_handle,
                      const char *image, uint32_t size);
static int list_endpoints(struct libusb_endpoint_descriptor endpoints[],
                          struct libusb_ss_endpoint_companion_descriptor ss_endpoints[],
                          libusb_device *device);


struct usb_device_id {
  uint16_t vid;
  uint16_t pid;
  int needs_firmware;
};


static struct usb_device_id usb_device_ids[] = {
  { 0x04b4, 0x00f3, 1 },     /* Cypress / FX3 Boot-loader */
  { 0x04b4, 0x00f1, 0 }      /* Cypress / FX3 Streamer Example */
};
static int n_usb_device_ids = sizeof(usb_device_ids) / sizeof(usb_device_ids[0]);


void usb_device_init()
{
  int ret = libusb_init(nullptr);
  if(ret < 0) {
    throw runtime_error(format("{} ({}:{}) ", __FUNCTION__, __FILE__, __LINE__) + libusb_error_name(ret) + " " + libusb_strerror(ret));
  }
}
void usb_device_destroy()
{
  libusb_exit(nullptr);
}


vector<USBDeviceInfo> usb_device_get_device_list()
{
  const int MAX_STRING_BYTES = 256;
  char temporary_string[MAX_STRING_BYTES];

  libusb_device **list;
  ssize_t nusbdevices = libusb_get_device_list(0, &list);
  if (nusbdevices < 0) {
    USB_ERROR_PRINTLN(TAG, nusbdevices);
    throw runtime_error("error");
  }

  vector<USBDeviceInfo> device_infos;
  int count = 0;
  for (ssize_t j = 0; j < nusbdevices; ++j) {
    libusb_device *device = list[j];
    struct libusb_device_descriptor desc;
    int ret = libusb_get_device_descriptor(device, &desc);
    for (int i = 0; i < n_usb_device_ids; ++i) {
      if (!(desc.idVendor == usb_device_ids[i].vid &&
            desc.idProduct == usb_device_ids[i].pid)) {
        continue;
      }

      USBDeviceInfo dev_info;
      dev_info.index = count;
      dev_info.usb_vendor_id  = desc.idVendor;
      dev_info.usb_product_id = desc.idProduct;
      dev_info.need_firmware  = usb_device_ids[i].needs_firmware;

      libusb_device_handle *dev_handle = 0;
      ret = libusb_open(device, &dev_handle);
      if (ret < 0) {
        USB_ERROR_PRINTLN(TAG, ret);
        goto FAIL2;
      }

      if (desc.iManufacturer) {
        ret = libusb_get_string_descriptor_ascii(dev_handle, desc.iManufacturer,
                      (unsigned char*)temporary_string, MAX_STRING_BYTES);
        if (ret < 0) {
          USB_ERROR_PRINTLN(TAG, ret);
          goto FAIL3;
        }

        dev_info.manufacturer = temporary_string;
      }

      if (desc.iProduct) {
        ret = libusb_get_string_descriptor_ascii(dev_handle, desc.iProduct,
                      (unsigned char*)temporary_string, MAX_STRING_BYTES);
        if (ret < 0) {
          USB_ERROR_PRINTLN(TAG, ret);
          goto FAIL3;
        }

        dev_info.product = temporary_string;
      }

      if (desc.iSerialNumber) {
        ret = libusb_get_string_descriptor_ascii(dev_handle, desc.iSerialNumber,
                      (unsigned char*)temporary_string, MAX_STRING_BYTES);
        if (ret < 0) {
          USB_ERROR_PRINTLN(TAG, ret);
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
      count++;
    }
  }

FAIL2:
  libusb_free_device_list(list, 1);

  return device_infos;
}


usb_device_t *usb_device_open(USBDeviceInfo index, const char* image,
                              uint32_t size)
{
  usb_device_t *ret_val = 0;
  libusb_context *ctx = 0;

  //libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

  libusb_device *device;
  int needs_firmware = 0;
  libusb_device_handle *dev_handle = find_usb_device(index, &device, &needs_firmware);
  if (dev_handle == 0) {
    ErrorPrintln(TAG, "Unable to open the USB device");
    return 0;
  }

  if (needs_firmware) {
    int ret = load_image(dev_handle, image, size);
    if (ret != 0) {
      ErrorPrintln(TAG, "Failed to load firmware image in the SDR");
      libusb_close(dev_handle);
      return 0;
    }

    /* rescan USB to get a new device handle */
    libusb_close(dev_handle);

    /* wait unitl firmware is ready */
    usleep(500 * 1000L);

    needs_firmware = 0;
    dev_handle = find_usb_device(index, &device, &needs_firmware);
    if (dev_handle == 0) {
      ErrorPrintln(TAG, "Unable to open the USB device after loading the firmware");
      return 0;
    }
    if (needs_firmware) {
      ErrorPrintln(TAG, "The USB device is still in boot loader mode");
      libusb_close(dev_handle);
      return 0;
    }
  }

  int speed = libusb_get_device_speed(device);
  if ( speed == LIBUSB_SPEED_LOW || speed == LIBUSB_SPEED_FULL || speed == LIBUSB_SPEED_HIGH ) {
      ErrorPrintln(TAG, "The USB device isn't capable of using USB 3.x SuperSpeed");
      libusb_close(dev_handle);
      return 0;
  }

  /* list endpoints */
  struct libusb_endpoint_descriptor endpoints[MAX_ENDPOINTS];
  struct libusb_ss_endpoint_companion_descriptor ss_endpoints[MAX_ENDPOINTS];
  int ret = list_endpoints(endpoints, ss_endpoints, device);
  if (ret < 0) {
    log_error("list_endpoints() failed", __func__, __FILE__, __LINE__);
    libusb_close(dev_handle);
    return 0;
  }
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
    fprintf(stderr, "ERROR - bulk in endpoint not found\n");
    libusb_close(dev_handle);
    return 0;
  }

  /* we are good here - create and initialize the usb_device */
  usb_device_t *t = (usb_device_t *) malloc(sizeof(usb_device_t));
  t->dev = device;
  t->dev_handle = dev_handle;
  t->context = ctx;
  t->completed = 0;
  t->nendpoints = nendpoints;
  memset(t->endpoints, 0, sizeof(t->endpoints));
  for (int i = 0; i < nendpoints; ++i) {
    t->endpoints[i] = endpoints[i];
    t->ss_endpoints[i] = ss_endpoints[i];
  }
  t->bulk_in_endpoint_address = bulk_in_endpoint_address;
  t->bulk_in_max_packet_size = bulk_in_max_packet_size;
  t->bulk_in_max_burst = bulk_in_max_burst;

  ret_val = t;
  return ret_val;
}


void usb_device_close(usb_device_t *t)
{
  libusb_close(t->dev_handle);
  free(t);
  return;
}


int usb_device_handle_events(usb_device_t *t)
{
  return libusb_handle_events_completed(t->context, &t->completed);
}

/**
 * @brief Send a request to the USb device
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
int usb_device_control(usb_device_t *t, uint8_t request, uint16_t value,
                       uint16_t index, uint8_t *data, uint16_t length, bool read) {

  const uint8_t bmWriteRequestType = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
  const uint8_t bmReadRequestType = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
  const unsigned int timeout = 5000;        // timeout (in ms) for each command
  int ret;

  if (!read) {
      ret = libusb_control_transfer(t->dev_handle, bmWriteRequestType,
                                    request, value, index, data, length,
                                    timeout);
      if (ret < 0) {
        USB_ERROR_PRINTLN(TAG, ret);
        return -1;
      }
  }
  else
  {
      ret = libusb_control_transfer(t->dev_handle, bmReadRequestType,
                                    request, value, index, data, length,
                                    timeout);
      // LIBUSB_ERROR_PIPE indicates that the device voluntarily closed
      // the connection, hence not an error
      if (ret < 0 && ret != LIBUSB_ERROR_PIPE) {
        USB_ERROR_PRINTLN(TAG, ret);
        return -1;
      }
  }

  return 0;
}



/* internal functions */
static libusb_device_handle *find_usb_device(USBDeviceInfo dev_select,
                             libusb_device **device, int *needs_firmware)
{
  *device = 0;
  *needs_firmware = 0;

  libusb_device **list = 0;
  ssize_t nusbdevices = libusb_get_device_list(NULL, &list);
  if (nusbdevices < 0) {
    USB_ERROR_PRINTLN(TAG, nusbdevices);
    return (libusb_device_handle *)0;
  }

  int count = 0;
  for (ssize_t j = 0; j < nusbdevices; ++j) {
    libusb_device *dev = list[j];
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(dev, &desc);
    for (int i = 0; i < n_usb_device_ids; ++i) {
      if (desc.idVendor == usb_device_ids[i].vid &&
        desc.idProduct == usb_device_ids[i].pid)
      {
        if (count == dev_select.index) {
          *device = dev;
          *needs_firmware = usb_device_ids[i].needs_firmware;

          // Crappy solution to keep the ref counter at the same level after the unref below
          libusb_ref_device(dev);
        }
        count++;
      }
    }
    libusb_unref_device(dev);
  }

  libusb_free_device_list(list, 0);

  if (*device == 0) {
    ErrorPrintln(TAG, "No USB device corresponds to the object given");
    return 0;
  }

  libusb_device_handle *dev_handle = 0;
  int ret = libusb_open(*device, &dev_handle);
  if (ret < 0) {
    USB_ERROR_PRINTLN(TAG, ret);
    return 0;
  }

#ifndef _WIN32
  ret = libusb_kernel_driver_active(dev_handle, 0);
  if (ret < 0) {
    libusb_close(dev_handle);
    USB_ERROR_PRINTLN(TAG, ret);
    return 0;
  }
  if (ret == 1) {
    libusb_close(dev_handle);
    ErrorPrintln(TAG, "A kernel driver is active on the device. This prevents use by SDDC_Driver");
    return 0;
  }
#endif

  ret = libusb_claim_interface(dev_handle, 0);
  if (ret < 0) {
    libusb_close(dev_handle);
    USB_ERROR_PRINTLN(TAG, ret);
    return 0;
  }

  return dev_handle;
}


int load_image(libusb_device_handle *dev_handle, const char *image, uint32_t image_size)
{
  int ret_val = -1;

  verbose = 1;

  ret_val = fx3_load_ram(dev_handle, image);
  return ret_val;
}

static int list_endpoints(struct libusb_endpoint_descriptor endpoints[],
                          struct libusb_ss_endpoint_companion_descriptor ss_endpoints[],
                          libusb_device *device)
{
  struct libusb_config_descriptor *config;
  int ret = libusb_get_active_config_descriptor(device, &config);
  if (ret < 0) {
    USB_ERROR_PRINTLN(TAG, ret);
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
        ret = libusb_get_ss_endpoint_companion_descriptor(0, endpoint,
                &endpoint_ss_companion);

        //printf("PktSize=%d\n", endpoint->wMaxPacketSize * (endpoint_ss_companion->bMaxBurst + 1));
        if (ret < 0 && ret != LIBUSB_ERROR_NOT_FOUND) {
          USB_ERROR_PRINTLN(TAG, ret);
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
