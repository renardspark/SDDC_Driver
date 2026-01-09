#ifndef _H_TYPES_CPP
#define _H_TYPES_CPP

#include <string>

using namespace std;

namespace SDDC {
	typedef struct DeviceItem {
		uint8_t index; ///< An arbitrary index used to uniquely identify each device
		string product; ///< The model of the SDR
		string serial_number; ///< The serial number of the SDR
	} DeviceItem;
}

#endif