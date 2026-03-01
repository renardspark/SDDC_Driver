#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
extern void usleep(__int64 usec);
#else
#include <unistd.h>
#endif

#include "FX3handler.h"
#include "usb_device.h"
#include "ezusb.h"
#include "firmware.h"

using namespace std;

#define firmware_data ((const char *)FIRMWARE)
#define firmware_size sizeof(FIRMWARE)

#define TAG "FX3Handler"

fx3class *CreateUsbHandler()
{
    TracePrintln(TAG, "");

    return new fx3handler();
}

fx3handler::fx3handler()
{
    TracePrintln(TAG, "");
}

fx3handler::~fx3handler()
{
    TracePrintln(TAG, "");

    Close();
}

bool fx3handler::Open(SDDC::DeviceItem dev_selector)
{
    TracePrintln(TAG, "*");

    if (usb_device_infos.size() == 0) {
        usb_device_infos = dev.getDeviceList();
    }

    if(dev_selector.index >= usb_device_infos.size())
    {
        ErrorPrintln(TAG, "The device request isn't part of the list");
        return false;
    }


    dev.open(usb_device_infos[dev_selector.index], firmware_data, firmware_size);
    DebugPrintln(TAG, "Open device with dev_index=%d", dev_selector.index);

    usleep(5000);
    Control(STOPFX3, (uint8_t)0);

    return true;
}

bool fx3handler::Close(void)
{
    TracePrintln(TAG, "");

    dev.close();

    return true;
}

bool fx3handler::Control(FX3Command command, uint8_t data)
{
    TracePrintln(TAG, "%d, %d", command, data);

    return dev.control(command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::Control(FX3Command command, uint32_t data)
{
    TracePrintln(TAG, "%d, %d", command, data);

    return dev.control(command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::Control(FX3Command command, uint64_t data)
{
    TracePrintln(TAG, "%d, %ld", command, data);

    return dev.control(command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::SetArgument(uint16_t index, uint16_t value)
{
    TracePrintln(TAG, "%d, %d", index, value);

    uint8_t data = 0;
    return dev.control(SETARGFX3, value, index, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::GetHardwareInfo(uint32_t *data)
{
    TracePrintln(TAG, "%p", data);
#ifdef _DEBUG
    uint8_t enable_debug = 1;
#else
    uint8_t enable_debug = 0;
#endif
    return dev.control(TESTFX3, enable_debug, 0, (uint8_t *)data, sizeof(*data), 1) == 0;
}

void fx3handler::StartStream(ringbuffer<int16_t> &samples_buf)
{
    TracePrintln(TAG, "");

    inputbuffer = &samples_buf;

    dev.streaming_open_async(inputbuffer->getBlockSize() * sizeof(int16_t), concurrentTransfers, PacketRead, this);
    //samples_buf.setBlockSize(dev.streaming_framesize() / sizeof(int16_t));

    DebugPrintln(TAG, "Samples buffer blocksize: %d", samples_buf.getBlockSize());

    // Start background thread to poll the events
    streamRunning = true;

    // FIXME: Will crash if streaming hasn't properly started
    dev.streaming_start();

    poll_thread = std::thread(
        [this]()
        {
            while (streamRunning)
            {
                dev.handleEvents();
            }
        }
    );
}

void fx3handler::StopStream()
{
    TracePrintln(TAG, "");

    streamRunning = false;

    poll_thread.join();

    dev.streaming_stop();
    dev.streaming_close();
}

void fx3handler::PacketRead(uint32_t data_size, uint8_t *data, void *context)
{
    TraceExtremePrintln(TAG, "%d, %p, %p", data_size, data, context);
    fx3handler *handler = (fx3handler *)context;

    assert(data_size == handler->inputbuffer->getBlockSize() * sizeof(int16_t));

    vector<int16_t> vec_data((int16_t*)data, (int16_t*)data + data_size / sizeof(int16_t));
    handler->inputbuffer->push(vec_data);
}

bool fx3handler::ReadDebugTrace(uint8_t *pdata, uint8_t len)
{
    TracePrintln(TAG, "%p, %d", pdata, len);

    return dev.control(READINFODEBUG, pdata[0], 0, (uint8_t *)pdata, len, 1) == 0;
}

size_t fx3handler::GetDeviceListLength()
{
    TracePrintln(TAG, "");

    if (usb_device_infos.size() == 0) {
        usb_device_infos = dev.getDeviceList();
    }

    return usb_device_infos.size();
}

vector<SDDC::DeviceItem> fx3handler::GetDeviceList()
{
    TracePrintln(TAG, "");

    vector<SDDC::DeviceItem> dev_list;

    usb_device_infos = dev.getDeviceList();

    for(auto it = usb_device_infos.begin(); it < usb_device_infos.end(); it++)
    {
        SDDC::DeviceItem dev = {
            .index = it->index,
            .product = it->product,
            .serial_number = it->serial_number
        };
        dev_list.push_back(dev);
    }

    return dev_list;
}