#include <SoapySDR/Device.hpp>
#include <SoapySDR/Logger.hpp>
#include <SoapySDR/Formats.h>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Formats.hpp>

#include <SoapySDR/Time.hpp>
#include <cstdint>
#include <cstring>
#include "SoapySDDC.hpp"

#define TAG "SoapySDDC_Streaming"

std::vector<std::string> SoapySDDC::getStreamFormats(const int, const size_t) const
{
    TracePrintln(TAG, "*, *");
    std::vector<std::string> formats;
    formats.push_back(SOAPY_SDR_CF32);
    return formats;
}

std::string SoapySDDC::getNativeStreamFormat(const int, const size_t, double &fullScale) const
{
    TracePrintln(TAG, "*, *, %f", fullScale);
    fullScale = 1.0;
    return SOAPY_SDR_CF32;
}

SoapySDR::ArgInfoList SoapySDDC::getStreamArgsInfo(const int, const size_t) const
{
    TracePrintln(TAG, "*, *");

    SoapySDR::ArgInfoList streamArgs;
    return streamArgs;
}

SoapySDR::Stream *SoapySDDC::setupStream(const int direction,
                                         const std::string &format,
                                         const std::vector<size_t> &channels,
                                         const SoapySDR::Kwargs&)
{
    TracePrintln(TAG, "%d, %s, *, *", direction, format.c_str());

    if (direction != SOAPY_SDR_RX)
        throw std::runtime_error("setupStream failed: SDDC only supports RX");
    if (channels.size() != 1)
        throw std::runtime_error("setupStream failed: SDDC only supports one channel");
    
    if (format == SOAPY_SDR_CF32)
    {
        SoapySDR_logf(SOAPY_SDR_INFO, "Using format CF32.");
    }
    else
    {
        throw std::runtime_error("setupStream failed: SDDC only supports CF32.");
    }

    // FIXME : The size of the buffers should be aligned directly on the size
    // of the output IQ buffer
    bytesPerSample = sizeof(sddc_complex_t);
    bufferLength = transferSamples / 2;

    DebugPrintln(TAG, "CF32 element size : %ld", SoapySDR::formatToSize(SOAPY_SDR_CF32));
    DebugPrintln(TAG, "Bytes per sample : %d", bytesPerSample);
    DebugPrintln(TAG, "Input buffer size : %ld (%ld bytes)", bufferLength, bufferLength * bytesPerSample);

    samples_block_write = 0;
    samples_block_read  = 0;
    _buf_count = 0;

    // allocate buffers
    samples_buffer.resize(numBuffers);
    for (auto &buff : samples_buffer)
        buff.reserve(bufferLength * bytesPerSample);
    for (auto &buff : samples_buffer)
        buff.resize(bufferLength * bytesPerSample);

    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return (SoapySDR::Stream *)this;
}

void SoapySDDC::closeStream(SoapySDR::Stream*)
{
    TracePrintln(TAG, "*");
    radio_handler->Stop();
}

size_t SoapySDDC::getStreamMTU(SoapySDR::Stream*) const
{
    TracePrintln(TAG, "*");
    return bufferLength;
}

int SoapySDDC::activateStream(SoapySDR::Stream*,
                              const int,
                              const long long,
                              const size_t)
{
    TracePrintln(TAG, "*, *, *, *");
    resetBuffer = true;
    bufferedElems = 0;
    radio_handler->Start(true);

    return 0;
}

int SoapySDDC::deactivateStream(SoapySDR::Stream*,
                                const int,
                                const long long)
{
    TracePrintln(TAG, "*, *, *");
    radio_handler->Stop();
    return 0;
}

int SoapySDDC::readStream(SoapySDR::Stream *stream,
                          void *const *buffs,
                          const size_t numElems,
                          int &flags,
                          long long &timeNs,
                          const long timeoutUs)
{
    TraceExtremePrintln(TAG, "%p, %p, %ld, %d, %lld, %ld", stream, buffs, numElems, flags, timeNs, timeoutUs);

    void *buffer_channel0 = buffs[0];
    if (bufferedElems == 0)
    {
        int ret = this->acquireReadBuffer(stream, _currentHandle, (const void **)&_currentBuff, flags, timeNs, timeoutUs);
        if (ret < 0)
            return ret;
        bufferedElems = ret;
    }

    size_t returnedElems = std::min(bufferedElems, numElems);

    // convert into user's buffer for channel 0
    std::memcpy(buffer_channel0, _currentBuff, returnedElems * bytesPerSample);

    // bump variables for next call into readStream
    bufferedElems -= returnedElems;
    _currentBuff += returnedElems * bytesPerSample;

    // return number of elements written to buff0
    if (bufferedElems != 0)
        flags |= SOAPY_SDR_MORE_FRAGMENTS;
    else
        this->releaseReadBuffer(stream, _currentHandle);
    return returnedElems;
}

int SoapySDDC::acquireReadBuffer(SoapySDR::Stream*,
                                 size_t &handle,
                                 const void **buffs,
                                 int &flags,
                                 long long&,
                                 const long timeoutUs)
{
    TraceExtremePrintln(TAG, "*, %ld, %p, *, *, %ld", handle, buffs, timeoutUs);

    if (resetBuffer)
    {
        samples_block_read = (samples_block_read + _buf_count.exchange(0)) % numBuffers;
        resetBuffer = false;
        _overflowEvent = false;
    }

    if (_overflowEvent)
    {
        samples_block_read = (samples_block_read + _buf_count.exchange(0)) % numBuffers;
        _overflowEvent = false;
        SoapySDR::log(SOAPY_SDR_SSI, "O");
        return SOAPY_SDR_OVERFLOW;
    }
    // wait for a buffer to become available
    if (_buf_count == 0)
    {
        std::unique_lock<std::mutex> lock(_buf_mutex);
        _buf_cond.wait_for(lock, std::chrono::microseconds(timeoutUs), [this]
                           { return _buf_count != 0; });
        if (_buf_count == 0)
            return SOAPY_SDR_TIMEOUT;
    }
    // extract handle and buffer
    handle = samples_block_read;
    samples_block_read = (samples_block_read + 1) % numBuffers;

    buffs[0] = (void *)samples_buffer[handle].data();
    flags = 0;

    // return number available
    return samples_buffer[handle].size() / bytesPerSample;
}

void SoapySDDC::releaseReadBuffer(SoapySDR::Stream*,
                                  const size_t)
{
    TraceExtremePrintln(TAG, "*, *");
    std::lock_guard<std::mutex> lock(_buf_mutex);
    _buf_count--;
}