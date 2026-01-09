# RadioHandler

RadioHandler is the top-level API to interact with the SDR from a C++ application.

## Device discovery

To get the list of devices currently available, RadioHandler provides the static function `GetDeviceList()`

This function return a vector containing `SDDC::DeviceItem` objects

Here is an example to show all devices available :
```cpp
vector<SDDC::DeviceItem> device_list = RadioHandler::GetDeviceList();

cout << "Index - Model - Serial number" << endl;
for(auto it: device_list)
{
	cout << std::to_string(it.index) << " - " << it.product << " - " << it.serial_number << endl;
}
```

## Initialization

A new instance of RadioHandler is created by using its constructor.

`RadioHandler::Init()` then needs to be called with an `SDDC::DeviceItem` object.

Example :
```cpp
vector<SDDC::DeviceItem> device_list = RadioHandler::GetDeviceList();

if(device_list.empty())
{
	cerr << "Device not found" << endl;
	return 1;
}

RadioHandler radio;
radio.Init(device_list[0]);
```

## Get/set parameters

The SDR can be controlled with the help of the getter and setter functions.
These function starts with `RadioHandler::Get*()` or `RadioHandler::Set*()`.

All of these parameters can be modified when the stream is running.

Example :
```cpp
const uint32_t frequency = 95300000;

// Get the best mode (usually between HF or VHF) to receive the frequency given
const sddc_rf_mode_t best_mode = radio.GetBestRFMode(frequency);
radio.SetRFMode(best_mode);

radio.SetADCSampleRate(10000000);
radio.SetCenterFrequency(frequency);
radio.SetBiasT_VHF(true);
```

## Start and use the stream

The data stream can be controlled using the functions `RadioHandler::Start()`, `RadioHandler::()`, `RadioHandler::Pause()`, and `RadioHandler::Resume()`.

The data can be retrieved as raw ADC values, or as an I/Q stream. Choosing between these two modes is done when calling `RadioHandler::Start()`.

To receive the resulting data buffers, a callback needs to be registered with `RadioHandler::AttachReal()`, `RadioHandler::AttachIQ()`

Example to get raw ADC streaam :
```cpp

void callback(void* user_context, const int16_t *data, uint32_t data_length)
{
	fwrite(data, data_length * sizeof(int16_t), 1, stdout);
}

radio.AttachReal(callback, /*user_context=*/nullptr);
radio.Start(false);
```

Example to get I/Q streaam :
```cpp

void callback(void* user_context, const sddc_complex_t *data, uint32_t data_length)
{
	fwrite(data, data_length * sizeof(sddc_complex_t), 1, stdout);
}

radio.AttachIQ(callback, /*user_context=*/nullptr);
radio.Start(true);
```