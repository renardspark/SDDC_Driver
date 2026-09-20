/*
 * This file is part of SDDC_Driver.
 *
 * =====================
 *      MIT License
 * =====================
 *
 * Copyright (C) 2020 - Oscar Steila
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

using namespace std;

#define R820T_FREQ (32000000)	// R820T reference frequency
#define R820T2_IF_CARRIER (4570000)

const char TAG[] = "BBRF103Radio";

const vector<float> BBRF103Radio::rf_steps_vhf =  {
    0.0f, 0.9f, 1.4f, 2.7f, 3.7f, 7.7f, 8.7f, 12.5f, 14.4f, 15.7f,
    16.6f, 19.7f, 20.7f, 22.9f, 25.4f, 28.0f, 29.7f, 32.8f,
    33.8f, 36.4f, 37.2f, 38.6f, 40.2f, 42.1f, 43.4f, 43.9f,
    44.5f, 48.0f, 49.6f
};

const vector<float> BBRF103Radio::if_steps_vhf =  {
    -4.7f, -2.1f, 0.5f, 3.5f, 7.7f, 11.2f, 13.6f, 14.9f, 16.3f, 19.5f, 23.1f, 26.5f, 30.0f, 33.7f, 37.2f, 40.8f
};

const vector<float> BBRF103Radio::rf_steps_hf = {
    -20.0f, -10.0f, 0.0f
};

BBRF103Radio::BBRF103Radio(fx3class* fx3):
    RadioHardware(fx3)
{
    
}

const array<float, 2> BBRF103Radio::GetADCSampleRateLimits()
{
    ErrorPrintln(TAG, "I don't know the limits for this device, please set them before using it. Feel free to open an issue if needed");
    return {0, 0};
}

sddc_rf_mode_t BBRF103Radio::GetBestRFMode(uint64_t freq)
{
    if (freq < 10 * 1000) return NOMODE;
    if (freq > 1750 * 1000 * 1000) return NOMODE;

    if ( freq >= this->sampleRate / 2)
        return VHFMODE;
    else
        return HFMODE;
}

sddc_err_t BBRF103Radio::SetRFMode(sddc_rf_mode_t mode)
{
    if (mode == VHFMODE)
    {
        // switch to VHF Attenna
        UnsetGPIO(ATT_SEL0 | ATT_SEL1);

        // Initialize Tuner
        return Fx3->Control(TUNERINIT, (uint32_t)R820T_FREQ) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
    }

    else if (mode == HFMODE )   // (mode == HFMODE || mode == VLFMODE) no more VLFMODE
    {
        // Stop Tuner
        Fx3->Control(TUNERSTDBY);

        // switch to HF Attenna
        return SetGPIO(ATT_SEL0 | ATT_SEL1) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
    }

    return ERR_NOT_COMPATIBLE;
}

sddc_err_t BBRF103Radio::SetRFAttenuation_HF(size_t att)
{
    if (att > 2) att = 2;

    switch (att)
    {
    case 1: //11
        gpios |= ATT_SEL0 | ATT_SEL1;
        break;
    case 0: //01
        gpios |=  ATT_SEL0;
        gpios &=  ~ATT_SEL1;
        break;
    case 2:   //10
    default:
        gpios |=  ATT_SEL1;
        gpios &=  ~ATT_SEL0;
        break;
    }
    return Fx3->Control(GPIOFX3, gpios) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}
sddc_err_t BBRF103Radio::SetRFAttenuation_VHF(uint16_t att)
{
    return Fx3->SetArgument(R82XX_ATTENUATOR, att) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

sddc_err_t BBRF103Radio::SetCenterFrequency_HF(uint32_t freq)
{
    freqLO_HF = freq;
    return ERR_NOT_COMPATIBLE;
}

uint32_t BBRF103Radio::GetTunerFrequency_HF()
{
    return 0;
}
uint32_t BBRF103Radio::GetTunerFrequency_VHF()
{
    return R820T2_IF_CARRIER;
}
sddc_err_t BBRF103Radio::SetCenterFrequency_VHF(uint32_t freq)
{
    if(!Fx3->Control(TUNERTUNE, freq))
        return ERR_FX3_TRANSFER_FAILED;

    freqLO_VHF = freq;

    return ERR_SUCCESS;
}

vector<float> BBRF103Radio::GetRFSteps_HF()
{
    return this->rf_steps_hf;
}
vector<float> BBRF103Radio::GetRFSteps_VHF()
{
    return this->rf_steps_vhf;
}

vector<float> BBRF103Radio::GetIFSteps_HF()
{
    return vector<float>();
}
vector<float> BBRF103Radio::GetIFSteps_VHF()
{
    return this->if_steps_vhf;
}

sddc_err_t BBRF103Radio::SetIFGain_HF(size_t)
{
    return ERR_NOT_COMPATIBLE;
}

sddc_err_t BBRF103Radio::SetIFGain_VHF(size_t attIndex)
{
    return Fx3->SetArgument(R82XX_VGA, (uint16_t)attIndex) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}