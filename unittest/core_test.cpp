#include "FX3Class.h"
#include "CppUnitTestFramework.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <inttypes.h>  // For portable 64-bit type printf codes

#include "RadioHandler.h"

using namespace std::chrono;

class fx3handler2 : public fx3class
{
    ~fx3handler2() {}

    bool Open(SDDC::DeviceItem)
    {
        return true;
    }

    bool Control(FX3Command command, uint8_t data = 0)
    {
        return true;
    }

    bool Control(FX3Command command, uint32_t data)
    {
        return true;
    }

    bool Control(FX3Command command, uint64_t data)
    {
        return true;
    }

    bool SetArgument(uint16_t index, uint16_t value)
    {
        return true;
    }

    bool GetHardwareInfo(uint32_t* data) {
        const uint8_t d[4] = {
            0, FIRMWARE_VER_MAJOR, FIRMWARE_VER_MINOR, 0
        };

        memcpy(data, d, 4);

        return true;
    }

    bool Enumerate(unsigned char& idx, char* lbuf)
    {
        return true;
    }

    bool ReadDebugTrace(uint8_t* pdata, uint8_t len) {
        return true;
    }

    std::thread emuthread;
    bool run;
	long nxfers;
    void StartStream(ringbuffer<int16_t>& input)
    {
        input.setBlockSize(transferSamples);
        run = true;
        emuthread = std::thread([&input, this]{
            while(run)
            {
                vector<int16_t> put(transferSamples, 0x5A5A);
                input.push(put);
                ++nxfers;
                std::this_thread::sleep_for(1ms);
            }
        });
    }

	void StopStream() {
        run = false;
        emuthread.join();
    }

    size_t GetDeviceListLength()
    {
        return 0;
    }

    bool GetDevice(unsigned char &a, char *b, size_t c, char *d, size_t e)
    {
        return true;
    }

    vector<SDDC::DeviceItem> GetDeviceList()
    {
        return vector<SDDC::DeviceItem>();
    }

    
public:
	long Xfers(bool clear) { long rv=nxfers; if (clear) nxfers=0; return rv; }


};

class testRadioHandler: public RadioHandler
{
public:
    testRadioHandler()
    {
        RadioHandler();
        fx3 = new fx3handler2();
    }

    static vector<SDDC::DeviceItem> GetDeviceList()
    {
        vector<SDDC::DeviceItem> devs;
        devs.push_back(SDDC::DeviceItem{
            .index = 0,
            .product = "Blank",
            .serial_number = "Blank"
        });
        return devs;
    }
};

static uint32_t frame_count;
static uint64_t totalsize;

static void Callback(void* context, const sddc_complex_t* data, uint32_t len)
{
    frame_count++;
    totalsize += len;
}

namespace {
    struct CoreFixture {};
}

TEST_CASE(CoreFixture, OpenTest)
{
    auto radio = new testRadioHandler();

    radio = new testRadioHandler();
    delete radio;
}

TEST_CASE(CoreFixture, BasicTest)
{
    vector<SDDC::DeviceItem> devices = testRadioHandler::GetDeviceList();
    auto radio = new testRadioHandler();
    radio->Init(devices[0]);

    radio->AttachIQ(Callback);

    REQUIRE_EQUAL(radio->getHardwareModel(), NORADIO);
    REQUIRE_EQUAL(radio->getHardwareName(), "Dummy");

    REQUIRE_EQUAL(radio->GetADCSampleRate(), 64000000u);
    radio->SetADCSampleRate(32000000);
    REQUIRE_EQUAL(radio->GetADCSampleRate(), 32000000u);

    // Test values out of bounds
    radio->SetADCSampleRate(0);
    REQUIRE_EQUAL(radio->GetADCSampleRate(), 1000000u);
    radio->SetADCSampleRate(128000000);
    REQUIRE_EQUAL(radio->GetADCSampleRate(), 64000000u);

    REQUIRE_EQUAL(radio->GetDither(), false);
    radio->SetDither(true);
    REQUIRE_EQUAL(radio->GetDither(), true);

    REQUIRE_EQUAL(radio->GetRand(), false);
    radio->SetRand(true);
    REQUIRE_EQUAL(radio->GetRand(), true);

    REQUIRE_EQUAL(radio->GetPGA(), false);
    radio->SetPGA(true);
    REQUIRE_EQUAL(radio->GetPGA(), true);

    REQUIRE_EQUAL(radio->GetBiasT_HF(), false);
    radio->SetBiasT_HF(true);
    REQUIRE_EQUAL(radio->GetBiasT_HF(), true);

    REQUIRE_EQUAL(radio->GetBiasT_VHF(), false);
    radio->SetBiasT_VHF(true);
    REQUIRE_EQUAL(radio->GetBiasT_VHF(), true);

    delete radio;
}

TEST_CASE(CoreFixture, R2IQTest)
{
    vector<SDDC::DeviceItem> devices = testRadioHandler::GetDeviceList();
    auto radio = new testRadioHandler();
    radio->Init(devices[0]);

    radio->AttachIQ(Callback);

    for (int decimate = 0; decimate < 5; decimate++)
    {
        frame_count = 0;
        totalsize = 0;
        radio->SetDecimation(decimate);
        radio->Start(true); // full bandwidth
        std::this_thread::sleep_for(1s);
        radio->Stop();

        REQUIRE_TRUE(frame_count > 0);
        REQUIRE_TRUE(totalsize > 0);
        REQUIRE_EQUAL(totalsize / frame_count, transferSamples/2);
    }

    delete radio;
}

TEST_CASE(CoreFixture, TuneTest)
{
    vector<SDDC::DeviceItem> devices = testRadioHandler::GetDeviceList();
    auto radio = new testRadioHandler();
    radio->Init(devices[0]);

    radio->AttachIQ(Callback);

    radio->SetDecimation(1); // full bandwidth
    radio->Start(true);

    for (uint64_t i = 1000; i < 15000000;  i+=377000)
    {
        radio->SetCenterFrequency(i);
        std::this_thread::sleep_for(0.011s);
    }

    radio->Stop();


    delete radio;
}
