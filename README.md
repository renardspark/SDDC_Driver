# SDDC_Driver

[![CMake](https://github.com/renardspark/SDDC_Driver/actions/workflows/cmake.yml/badge.svg)](https://github.com/renardspark/SDDC_Driver/actions/workflows/cmake.yml)

#### A set of drivers and tools for the RX888 MkII and its variants (BBRF103, HF103, RX888...)

This project includes the following components :
- **/Core** : The main library controlling the SDR from a computer
  - **RadioHandler** : The API exposed by the Core module. Suitable to interact directly with the SDR from C++ programs
- **/sddc-cli** : A command line tool to use your SDR easily
- **/ExtIO_sddc** : The compatibility layer for ExtIO.dll and HDSDR
- **/SoapySDDC** : The compatibility layer for [SoapySDR](https://github.com/pothosware/SoapySDR/wiki)
- **/libsddc** : A wrapper for C-based programs to interact directly with the SDR
- **/SDDC_FX3** : The firmware source code of the BBRF103 and others


## Differences with ExtIO_SDDC

This project could not exist without the work of **[Oscar Steila (ik1xpv)](https://github.com/ik1xpv)** and others towards the development of **the [original driver (ExtIO_SDDC)](https://github.com/ik1xpv/ExtIO_sddc)**.

This fork brings the following improvements on top of ExtIO_SDDC :
- Make Linux support the priority
- Extend the features offered by libsddc and SoapySDDC
- Provide a standalone tool (sddc-cli) to get samples from your SDR
- Use any ADC sampling rate (ExtIO_SDDC only allows preconfigured choices)
- Use libusb for all operating systems (no Cypress driver required)
- Documentation (WIP) of the internal APIs
- Resolve a handful of crashs
- Improve memory safety
- A deep refactor to better isolate each component and make the codebase more consistent

Those changes comes with downsides :
- Windows support is considered unstable (compilation works, but I've not tested with real hardware)
- The performance can be worse as a result of the refactor
- This driver cannot be used alongside ExtIO_SDDC on Windows computers.
  This is because ExtIO_SDDC uses the Cypress driver, while SDDC_Driver embeds its own driver.
  This issue is not present on other OSes.


## Getting started

You can download the latest binaries from the releases: https://github.com/renardspark/SDDC_Driver/releases.

If you want to give a try to the most recent build, the binaries are available [on Github Actions](https://github.com/renardspark/SDDC_Driver/actions/workflows/cmake.yml).

### Windows

**Warning** : Before using your SDR on Windows, drivers install is required to make it work.
The procedure is explained in [Windows_driver_setup.md](blob/master/Windows_driver_setup.md)

The ExtIO DLL is available for Windows (32 and 64 bit). It can be used with compatible SDRs such as HDSDR or SDR#.

You need to download **32bit version** of [fftw](http://www.fftw.org/install/windows.html) and [libusb](https://libusb.info/), and copy them to the same folder as ExtIO DLL.

### Linux / MacOS

SoapySDR support is available on Linux / MacOS. It can be used with SDRs such as Gqrx or SDRangel.


## Build Instructions for Core, sddc-cli, ExtIO_sddc, SoapySDDC and libsddc

### Windows

1. Install Visual Studio 2026 with Visual C++ support. You can use the free community version, which can be downloaded from: https://visualstudio.microsoft.com/downloads/
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
VS2022: >cmake -S . -B build/ -G "Visual Studio 18 2026" -A Win32
VS2022: >cmake -S . -B build/ -G "Visual Studio 17 2022" -A Win32
VS2019: >cmake -S . -B build/ -G "Visual Studio 16 2019" -A Win32
```

### Linux

1. Install CMake 3.19+ and development packages:
```bash
> sudo apt install cmake libfftw3-dev libusb-1.0-0-dev
```

2. Run the following commands in the root folder of the cloned repo:
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