#ifndef FX3HANDLER_H
#define FX3HANDLER_H

#include "../../config.h"

#define	VENDOR_ID     (0x04B4)
#define	STREAMER_ID   (0x00F1)
#define	BOOTLOADER_ID (0x00F3)

#include "../../FX3Class.h"
#include "usb_device.h"
#include "streaming.h"
#include "../../dsp/ringbuffer.h"

using namespace std;

class fx3handler : public fx3class
{
public:
	fx3handler();
	virtual ~fx3handler(void);
	bool Open(uint8_t dev_index) override;
	bool Control(FX3Command command, uint8_t data) override;
	bool Control(FX3Command command, uint32_t data) override;
	bool Control(FX3Command command, uint64_t data) override;
	bool SetArgument(uint16_t index, uint16_t value) override;
	bool GetHardwareInfo(uint32_t* data) override;
	bool ReadDebugTrace(uint8_t* pdata, uint8_t len) override;
	void StartStream(ringbuffer<int16_t>& input) override;
	void StopStream() override;
	size_t GetDeviceListLength() override;
	bool GetDevice(unsigned char &idx, char *name, size_t name_len, char *serial, size_t serial_len) override;
	vector<SDDC::DeviceItem> GetDeviceList() override;

private:
	bool Close(void);

	static void PacketRead(uint32_t data_size, uint8_t *data, void *context);

	std::vector<USBDeviceInfo> usb_device_infos;
	usb_device_t *dev;
	streaming_t *stream;
	ringbuffer<int16_t> *inputbuffer;
    bool streamRunning = false;
    std::thread poll_thread;
};


#endif // FX3HANDLER_H
