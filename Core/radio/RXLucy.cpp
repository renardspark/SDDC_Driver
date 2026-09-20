/*
 * This file is part of SDDC_Driver.
 *
 * =====================
 *      MIT License
 * =====================
 *
 * Copyright (C) 2021 - Oscar Steila
 * Copyright (C) 2021 - Howard Su
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

using namespace std;

#define ADC_FREQ (64u*1000*1000)
#define IF_FREQ (ADC_FREQ / 4)

#define HIGH_MODE 0x80
#define LOW_MODE 0x00

#define MODE HIGH_MODE

const char TAG[] = "RXLucyRadio";

RXLucyRadio::RXLucyRadio(fx3class *fx3)
    : RadioHardware(fx3)
{
    // initialize steps
    for(auto it = if_steps_hf.begin(); it != if_steps_hf.end(); it++)
    {
        int index = it - if_steps_hf.begin();
        int rf_scale = if_steps_hf.size() - index - 1;
        *it = -(
            ((rf_scale & 0x01) != 0) * 0.5f +
            ((rf_scale & 0x02) != 0) * 1.0f +
            ((rf_scale & 0x04) != 0) * 2.0f +
            ((rf_scale & 0x08) != 0) * 4.0f +
            ((rf_scale & 0x010) != 0) * 8.0f +
            ((rf_scale & 0x020) != 0) * 16.0f);
    }

    for (auto it = rf_steps_hf.begin(); it != rf_steps_hf.end(); it++)
    {
        int index = it - rf_steps_hf.begin();
        *it = -1.0f * (rf_steps_hf.size() - index - 1);
    }
}

const array<float, 2> RXLucyRadio::GetADCSampleRateLimits()
{
    ErrorPrintln(TAG, "I don't know the limits for this device, please set them before using it. Feel free to open an issue if needed");
    return {0, 0};
}


sddc_rf_mode_t RXLucyRadio::GetBestRFMode(uint64_t freq)
{
    if (freq < 35000ll * 1000) return NOMODE;
    if (freq > 6000ll * 1000 * 1000) return NOMODE;

    if ( freq >= this->sampleRate / 2)
        return VHFMODE;
    else
        return HFMODE;
}


sddc_err_t RXLucyRadio::SetRFAttenuation_HF(size_t att)
{
    if (att >= rf_steps_hf.size()) att = rf_steps_hf.size() - 1;

    uint8_t d = rf_steps_hf.size() - att - 1;

    return Fx3->SetArgument(VHF_ATTENUATOR, d) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}
sddc_err_t RXLucyRadio::SetRFAttenuation_VHF(uint16_t)
{
    return ERR_NOT_COMPATIBLE;
}

sddc_err_t RXLucyRadio::SetIFGain_HF(size_t att)  //HF103 now
{
    if (att >= if_steps_hf.size()) att = if_steps_hf.size() - 1;

    uint8_t d = if_steps_hf.size() - att - 1;

    return Fx3->SetArgument(DAT31_ATT, d) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}
sddc_err_t RXLucyRadio::SetIFGain_VHF(size_t)
{
    return ERR_NOT_COMPATIBLE;
}

uint32_t RXLucyRadio::GetTunerFrequency_HF()
{
    return 0;
}
sddc_err_t RXLucyRadio::SetCenterFrequency_HF(uint32_t)
{
    return ERR_NOT_COMPATIBLE;
}
uint32_t RXLucyRadio::GetTunerFrequency_VHF()
{
    return IF_FREQ;
}
sddc_err_t RXLucyRadio::SetCenterFrequency_VHF(uint32_t freq)
{
    if(!Fx3->Control(TUNERTUNE, freq + IF_FREQ))
        return ERR_FX3_TRANSFER_FAILED;

    freqLO_VHF = freq;

    return ERR_SUCCESS;
}

sddc_err_t RXLucyRadio::SetRFMode(sddc_rf_mode_t mode)
{
    if (mode == VHFMODE)
    {
        // switch to VHF Attenna
        sddc_err_t ret = SetGPIO(VHF_EN);
        if(ret != ERR_SUCCESS) return ret;

        // Initialize VCO

        // Initialize Mixer
        return Fx3->Control(TUNERINIT, (uint32_t)0) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
    }
    else if (mode == HFMODE)
    {
        if(!Fx3->Control(TUNERSTDBY))
            return ERR_FX3_TRANSFER_FAILED;

        return UnsetGPIO(VHF_EN);                // switch to HF Attenna
    }
    return ERR_NOT_COMPATIBLE;
}

vector<float> RXLucyRadio::GetRFSteps_HF()
{
    return this->rf_steps_hf;
}
vector<float> RXLucyRadio::GetRFSteps_VHF()
{
    return vector<float>();
}

vector<float> RXLucyRadio::GetIFSteps_HF()
{
    return this->if_steps_hf;
}
vector<float> RXLucyRadio::GetIFSteps_VHF()
{
    return vector<float>();
}


