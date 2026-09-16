#ifndef _H_TYPES_CPP
#define _H_TYPES_CPP

#include <string>

namespace SDDC {
	typedef struct DeviceItem {
		uint8_t index; ///< An arbitrary index used to uniquely identify each device
		std::string product; ///< The model of the SDR
		std::string serial_number; ///< The serial number of the SDR
	} DeviceItem;
}

#endif