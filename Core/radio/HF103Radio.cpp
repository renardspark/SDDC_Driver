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

using namespace std;

const char TAG[] = "HF103Radio";

HF103Radio::HF103Radio(fx3class* fx3)
    : RadioHardware(fx3)
{
    // initialize steps
    for (uint8_t i = 0 ; i < step_size; i++) {
        this->rf_steps_hf[step_size - i - 1] = -(
            ((i & 0x01) != 0) * 0.5f +
            ((i & 0x02) != 0) * 1.0f +
            ((i & 0x04) != 0) * 2.0f +
            ((i & 0x08) != 0) * 4.0f +
            ((i & 0x010) != 0) * 8.0f +
            ((i & 0x020) != 0) * 16.0f
        );
    }
}

const array<float, 2> HF103Radio::GetADCSampleRateLimits()
{
    ErrorPrintln(TAG, "I don't know the limits for this device, please set them before using it. Feel free to open an issue if needed");
    return {0, 0};
}

sddc_rf_mode_t HF103Radio::GetBestRFMode(uint64_t freq)
{
    if(freq > 32 * 1000 * 1000) return NOMODE;

    return HFMODE;
}

sddc_err_t HF103Radio::SetRFMode(sddc_rf_mode_t mode)
{
    if (mode != HFMODE)
        return ERR_NOT_COMPATIBLE;

    currentRFMode = mode;
    return ERR_SUCCESS;
}

sddc_err_t HF103Radio::SetCenterFrequency_HF(uint32_t freq)
{
    freqLO_HF = freq;
    return ERR_SUCCESS;
}
uint32_t HF103Radio::GetTunerFrequency_HF()
{
    return freqLO_HF;
}
sddc_err_t HF103Radio::SetCenterFrequency_VHF(uint32_t)
{
    return ERR_NOT_COMPATIBLE;
}
uint32_t HF103Radio::GetTunerFrequency_VHF()
{
    return 0;
}

sddc_err_t HF103Radio::SetRFAttenuation_HF(size_t att)
{
    if (att > step_size - 1) att = step_size - 1;

    uint8_t d = step_size - att - 1;

    return Fx3->SetArgument(DAT31_ATT, d) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}
sddc_err_t HF103Radio::SetRFAttenuation_VHF(uint16_t)
{
    return ERR_NOT_COMPATIBLE;
}

vector<float> HF103Radio::GetRFSteps_HF()
{
    return this->rf_steps_hf;
}
vector<float> HF103Radio::GetRFSteps_VHF()
{
    return vector<float>();
}

vector<float> HF103Radio::GetIFSteps_HF ()
{
    return vector<float>();
}
vector<float> HF103Radio::GetIFSteps_VHF()
{
    return vector<float>();
}

sddc_err_t HF103Radio::SetIFGain_HF  (size_t)
{
    return ERR_NOT_COMPATIBLE;
}
sddc_err_t HF103Radio::SetIFGain_VHF (size_t)
{
    return ERR_NOT_COMPATIBLE;
}
