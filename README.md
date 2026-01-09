# SDDC_Driver

[![CMake](https://github.com/renardspark/SDDC_Driver/actions/workflows/cmake.yml/badge.svg)](https://github.com/renardspark/SDDC_Driver/actions/workflows/cmake.yml)

#### A set of drivers and tools for the BBRF103 and its derivatives (HF103, RX888, RX888r2...)

This project includes the following components :
- **/Core** : The main library controlling the SDR from a computer
  - **RadioHandler** : The API exposed by the Core module. Suitable to interact directly with the SDR from C++ programs
- **/ExtIO_sddc** : The compatibility layer for ExtIO.dll and HDSDR
- **/SoapySDDC** : The compatibility layer for [SoapySDR](https://github.com/pothosware/SoapySDR/wiki)
- **/libsddc** : A wrapper for C-based programs to interact directly with the SDR
- **/SDDC_FX3** : The firmware source code of the BBRF103 and others


## Disclaimer

This project could not exist without the work of **[Oscar Steila (ik1xpv)](https://github.com/ik1xpv)** and others towards the development of the official BBRF103 (and derivatives) drivers in the **[ExtIO_sddc](https://github.com/ik1xpv/ExtIO_sddc)** repository. I'm truly thankful to all these contributors, who helped ExtIO_sddc go this far !

SDDC_Driver is my attempt as deeply rewriting the code of ExtIO_sddc in order to make it more clean and intuitive in my view.
The objective is also to better separate the main driver code (Core module) from the ExtIO module, which was the only module available at the start of ExtIO_sddc.

**Warning** : Consider this fork as no longer compatible with Windows, as I've not tried it on this platform. On the other end, Linux and MacOS should be fine.


## Getting started

You can download the latest binaries from the releases: https://github.com/renardspark/SDDC_Driver/releases.

If you want to give a try to the most recent build, the binaries are available [on Github Actions](https://github.com/renardspark/SDDC_Driver/actions/workflows/cmake.yml).


### Linux / MacOS

SoapySDR support is available on Linux / MacOS. It can be used with SDRs such as Gqrx or SDRangel.


## Build Instructions for Core, sddc-cli, ExtIO_sddc, SoapySDDC and libsddc

### Windows

1. Install Visual Studio 2019 with Visual C++ support. You can use the free community version, which can be downloaded from: https://visualstudio.microsoft.com/downloads/
1. Install CMake 3.19+, https://cmake.org/download/
1. Running the following commands in the root folder of the cloned repro:
```bash
> mkdir build
> cmake -S . -B build/
> cmake --build build/
or
> cmake --build build/ --config Release
or
> cmake --build build/ --config RelWithDebInfo
```

* If you are running **64bit** OS, you need to run the following different commands instead of "cmake .." based on your Visual Studio Version:
```
VS2022: >cmake .. -G "Visual Studio 17 2022" -A Win32
VS2019: >cmake .. -G "Visual Studio 16 2019" -A Win32
VS2017: >cmake .. -G "Visual Studio 15 2017 Win32"
VS2015: >cmake .. -G "Visual Studio 14 2015 Win32"
```

### Linux

1. Install CMake 3.19+
2. Install development packages:
```bash
> sudo apt install libfftw3-dev libusb-1.0-0-dev
```

3. Run the following commands in the root folder of the cloned repo:
```bash
> mkdir build
> cmake -S . -B build/
> cmake --build build/
or
> cmake --build build/ --config Release
or
> cmake --build build/ --config RelWithDebInfo
```

## Build Instructions for SDDC_FX3

- download latest Cypress EZ-USB FX3 SDK from here: https://www.cypress.com/documentation/software-and-drivers/ez-usb-fx3-software-development-kit
- follow the installation instructions in the PDF document 'Getting Started with FX3 SDK'; on Windows the default installation path will be 'C:\Program Files (x86)\Cypress\EZ-USB FX3 SDK\1.3' (see pages 17-18) - on Linux the installation path could be something like '/opt/Cypress/cyfx3sdk'
- add the following environment variables:
```
export FX3FWROOT=<installation path>
export ARMGCC_INSTALL_PATH=<ARM GCC installation path>
export ARMGCC_VERSION=4.8.1
```
(on Linux you may want to add those variables to your '.bash_profile' or '.profile')
- all the previous steps need to be done only once (or when you want to upgrade the version of the Cypress EZ-USB FX3 SDK)
- to compile the firmware run:
```
cd SDDC_FX3
make
```

## References
- EXTIO Specification from http://www.sdradio.eu/weaksignals/bin/Winrad_Extio.pdf
- Discussion and Support https://groups.io/g/NextGenSDRs
- Recommended Application http://www.weaksignals.com/
- http://www.hdsdr.de
- http://booyasdr.sourceforge.net/
- http://www.cypress.com/


#### Many thanks to all the contributors of [ExtIO_sddc](https://github.com/ik1xpv/ExtIO_sddc) !