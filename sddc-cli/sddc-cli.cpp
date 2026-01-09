#include <iostream>
#include <argparse/argparse.hpp>
#include "RadioHandler.h"

using namespace std;


void frame_cb(void *context, const int16_t *data, uint32_t length)
{
	fwrite(data, length * sizeof(int16_t), 1, stdout);
}

void frame_cb_iq(void *context, const sddc_complex_t *data, uint32_t length)
{
	fwrite(data, length * sizeof(sddc_complex_t), 1, stdout);
}

int main(int argc, char const *argv[])
{
	argparse::ArgumentParser program("sddc-cli");

	argparse::ArgumentParser list_command("list");
	list_command.add_description("List all SDDC devices connected");

	argparse::ArgumentParser listen_command("listen");
	listen_command.add_description("Start the device and output data stream");
	listen_command.add_argument("device")
		.help("Index of the device to use")
		.scan<'i', uint8_t>();
	listen_command.add_argument("sample-rate").scan<'i', uint32_t>();
	listen_command.add_argument("frequency")
		.help("Center frequency to tune to")
		.scan<'i', uint32_t>();
	listen_command.add_argument("-i", "--output-iq").flag();

	program.add_subparser(list_command);
	program.add_subparser(listen_command);

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		return 1;
	}

	if(program.is_subcommand_used("list"))
	{
		vector<SDDC::DeviceItem> device_list = RadioHandler::GetDeviceList();

		cout << "Index - Model - Serial number" << endl;
		for(auto it: device_list)
		{
			cout << std::to_string(it.index) << " - " << it.product << " - " << it.serial_number << endl;
		}
	}
	else if(program.is_subcommand_used("listen"))
	{
		const uint8_t device_index = listen_command.get<uint8_t>("device");
		const uint32_t frequency = listen_command.get<uint32_t>("frequency");

		vector<SDDC::DeviceItem> device_list = RadioHandler::GetDeviceList();

		if(device_list.size() <= device_index)
		{
			cerr << "Device not found" << endl;
			return 1;
		}

		RadioHandler radio;

		radio.Init(device_list[device_index]);

		radio.SetADCSampleRate(listen_command.get<uint32_t>("sample-rate"));
		radio.SetRFMode(radio.GetBestRFMode(frequency));
		radio.SetCenterFrequency(frequency);

		if(listen_command["--output-iq"] == true)
		{
			radio.AttachIQ(frame_cb_iq);
			radio.Start(true);
		}
		else
		{
			radio.AttachReal(frame_cb);
			radio.Start(false);
		}

		while(1);
	}


	// sddc_err_t Init(SDDC::DeviceItem dev_index);
	// sddc_err_t AttachReal(void (*callback)(void* context, const int16_t*, uint32_t), void* context = nullptr);
	// sddc_err_t AttachIQ(void (*callback)(void* context, const sddc_complex_t*, uint32_t), void* context = nullptr);
	// sddc_err_t Start(bool convert_r2iq);
	// sddc_err_t Stop();
	// sddc_err_t Pause();
	// sddc_err_t Resume();

	// // --- r2iq --- //
	// sddc_err_t	SetDecimation(uint8_t decimate);

	return 0;
}