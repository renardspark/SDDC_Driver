/*
 * This file is part of SDDC_Driver.
 *
 * =====================
 *      MIT License
 * =====================
 *
 * Copyright (C) 2020 - Howard Su
 * Copyright (C) 2025 - RenardSpark
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "RadioHardware.h"
#include <cstdio>

using namespace std;

#define TAG "RadioHardware"

sddc_rf_mode_t RadioHardware::GetRFMode()
{
    TracePrintln(TAG, "");
    return currentRFMode;
}

uint32_t RadioHardware::GetADCSampleRate()
{
    TracePrintln(TAG, "");
    return sampleRate;
}
sddc_err_t RadioHardware::SetADCSampleRate(uint32_t adc_rate)
{
    TracePrintln(TAG, "%d", adc_rate);

    if(sampleRate == adc_rate)
        return ERR_SUCCESS;

    const array<float, 2> limits = GetADCSampleRateLimits();
    if(adc_rate < limits[0]) adc_rate = limits[0];
    if(adc_rate > limits[1]) adc_rate = limits[1];
    sampleRate = adc_rate;
    return Fx3->Control(STARTADC, adc_rate) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

// --- Gain --- //
uint16_t RadioHardware::GetRF_HF()
{
    TracePrintln(TAG, "");
    return attenuationHFStep;
}
uint16_t RadioHardware::GetRF_VHF()
{
    TracePrintln(TAG, "");
    return attenuationVHFStep;
}
uint16_t RadioHardware::GetIF_HF()
{
    TracePrintln(TAG, "");
    return gainHFStep;
}
uint16_t RadioHardware::GetIF_VHF()
{
    TracePrintln(TAG, "");
    return gainVHFStep;
}

// --- Misc --- //
bool RadioHardware::GetDither()
{
    TracePrintln(TAG, "");
    return stateDither;
}
sddc_err_t RadioHardware::SetDither(bool new_state)
{
    TracePrintln(TAG, "%s", new_state ? "on" : "off");

    stateDither = new_state;
    if (stateDither)
        return SetGPIO(DITH);
    else
        return UnsetGPIO(DITH);
}

bool RadioHardware::GetPGA()
{
    TracePrintln(TAG, "");
    return statePGA;
}
sddc_err_t RadioHardware::SetPGA(bool new_state)
{
    TracePrintln(TAG, "%s", new_state ? "on" : "off");

    statePGA = new_state;
    if (statePGA)
        return SetGPIO(PGA_EN);
    else
        return UnsetGPIO(PGA_EN);
}

bool RadioHardware::GetRand()
{
    TracePrintln(TAG, "");
    return stateRand;
}
sddc_err_t RadioHardware::SetRand(bool new_state)
{
    TracePrintln(TAG, "%s", new_state ? "on" : "off");

    stateRand = new_state;
    if (stateRand)
        return SetGPIO(RANDO);
    else
        return UnsetGPIO(RANDO);
}

// ----- Bias T ----- //
bool RadioHardware::GetBiasT_HF()
{
    TracePrintln(TAG, "");
    return stateBiasT_HF;
}
sddc_err_t RadioHardware::SetBiasT_HF(bool new_state) 
{
    TracePrintln(TAG, "%s", new_state ? "on" : "off");

    stateBiasT_HF = new_state;
    if (stateBiasT_HF)
        return SetGPIO(BIAS_HF);
    else
        return UnsetGPIO(BIAS_HF);
}

bool RadioHardware::GetBiasT_VHF()
{
    TracePrintln(TAG, "");
    return stateBiasT_VHF;
}
sddc_err_t RadioHardware::SetBiasT_VHF(bool new_state)
{
    TracePrintln(TAG, "%s", new_state ? "on" : "off");

    stateBiasT_VHF = new_state;
    if (stateBiasT_VHF)
        return SetGPIO(BIAS_VHF);
    else
        return UnsetGPIO(BIAS_VHF);
}
// ----- //

// ----- Tuner ----- //
uint32_t RadioHardware::GetCenterFrequency_HF()
{
    TracePrintln(TAG, "");
    return freqLO_HF;
}
uint32_t RadioHardware::GetCenterFrequency_VHF()
{
    TracePrintln(TAG, "");
    return freqLO_VHF;
}

// ----- GPIOs ----- //
sddc_err_t RadioHardware::SetGPIO(uint32_t mask)
{
    TracePrintln(TAG, "%04X", mask);

    if((gpios | mask) == gpios)
        return ERR_SUCCESS;

    gpios |= mask;
    return Fx3->Control(GPIOFX3, gpios) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

sddc_err_t RadioHardware::UnsetGPIO(uint32_t mask)
{
    TracePrintln(TAG, "%04X", mask);

    if((gpios & ~mask) == gpios)
        return ERR_SUCCESS;

    gpios &= ~mask;
    return Fx3->Control(GPIOFX3, gpios) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

/**
 * @brief Change the state of an LED
 * 
 * @param[in] led The LED to change
 * @param[in] on  The new LED state
 */
sddc_err_t RadioHardware::SetLED(sddc_leds_t led, bool on)
{
    TracePrintln(TAG, "%X, %s", led, on ? "on" : "off");
    int pin;
    switch(led)
    {
        case sddc_leds_t::SDDC_LED_YELLOW:
            pin = LED_YELLOW;
            break;
        case sddc_leds_t::SDDC_LED_RED:
            pin = LED_RED;
            break;
        case sddc_leds_t::SDDC_LED_BLUE:
            pin = LED_BLUE;
            break;
        default:
            return ERR_NOT_LED;
    }

    if (on)
        return SetGPIO(pin);
    else
        return UnsetGPIO(pin);
}
// ----- //

RadioHardware::~RadioHardware()
{
    TracePrintln(TAG, "");
    if (Fx3) {
        SetGPIO(SHDWN);
    }
}