/*
 * This file is part of AtracDEnc.
 *
 * AtracDEnc is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * AtracDEnc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with AtracDEnc; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

#include <algorithm>

#include "../config.h"

template<class TPCM, int nIn>
class TQmf {
    static const float TapHalf[24];
    TFloat QmfWindow[48];
    TPCM PcmBuffer[nIn + 46];
    TFloat PcmBufferMerge[nIn + 46];
    TFloat DelayBuff[46];
public:
    TQmf() {
        const int sz = sizeof(QmfWindow)/sizeof(QmfWindow[0]);

        for (size_t i = 0; i < sz/2; i++) {
            QmfWindow[i] = QmfWindow[sz - 1 - i] = TapHalf[i] * 2.0;
        }
        std::fill(std::begin(PcmBuffer), std::end(PcmBuffer), 0);
        std::fill(std::begin(PcmBufferMerge), std::end(PcmBufferMerge), 0);
    }

    void Split(TPCM* in, TFloat* lower, TFloat* upper) {
        // Combine memory copy operations
        std::memmove(PcmBuffer, PcmBuffer + nIn, 46 * sizeof(TPCM));
        std::memcpy(PcmBuffer + 46, in, nIn * sizeof(TPCM));

        for (size_t j = 0; j < nIn; j += 2) {
            TFloat lowerSum = 0.0;
            TFloat upperSum = 0.0;

            // Unroll the loop to reduce loop control overhead
            for (size_t i = 0; i < 24; i += 4) {
                lowerSum += QmfWindow[2 * i] * PcmBuffer[48 - 1 + j - (2 * i)];
                upperSum += QmfWindow[(2 * i) + 1] * PcmBuffer[48 - 1 + j - (2 * i) - 1];

                lowerSum += QmfWindow[2 * (i + 1)] * PcmBuffer[48 - 1 + j - (2 * (i + 1))];
                upperSum += QmfWindow[(2 * (i + 1)) + 1] * PcmBuffer[48 - 1 + j - (2 * (i + 1)) - 1];

                lowerSum += QmfWindow[2 * (i + 2)] * PcmBuffer[48 - 1 + j - (2 * (i + 2))];
                upperSum += QmfWindow[(2 * (i + 2)) + 1] * PcmBuffer[48 - 1 + j - (2 * (i + 2)) - 1];

                lowerSum += QmfWindow[2 * (i + 3)] * PcmBuffer[48 - 1 + j - (2 * (i + 3))];
                upperSum += QmfWindow[(2 * (i + 3)) + 1] * PcmBuffer[48 - 1 + j - (2 * (i + 3)) - 1];
            }

            lower[j / 2] = lowerSum;
            upper[j / 2] = upperSum;

            TFloat temp = upper[j / 2];
            upper[j / 2] = lower[j / 2] - upper[j / 2];
            lower[j / 2] += temp;
        }
    }


    void Merge(TPCM* out, TFloat* lower, TFloat* upper) {
        std::copy(DelayBuff, DelayBuff + 46, PcmBufferMerge);
        TFloat* newPart = &PcmBufferMerge[46];

        // Unroll the loop to optimize the addition and subtraction
        for (int i = 0; i < nIn; i += 4) {
            newPart[i] = lower[i / 2] + upper[i / 2];
            newPart[i + 1] = lower[i / 2] - upper[i / 2];
            newPart[i + 2] = lower[i / 2 + 1] + upper[i / 2 + 1];
            newPart[i + 3] = lower[i / 2 + 1] - upper[i / 2 + 1];
        }

        TFloat* winP = PcmBufferMerge;
        for (size_t j = nIn / 2; j != 0; j--) {
            TFloat s1 = 0;
            TFloat s2 = 0;

            // Unroll the loop to reduce loop control overhead
            for (size_t i = 0; i < 48; i += 4) {
                s1 += winP[i] * QmfWindow[i];
                s2 += winP[i + 1] * QmfWindow[i + 1];

                s1 += winP[i + 2] * QmfWindow[i + 2];
                s2 += winP[i + 3] * QmfWindow[i + 3];
            }

            out[0] = s2;
            out[1] = s1;
            winP += 2;
            out += 2;
        }
        std::copy(PcmBufferMerge + nIn, PcmBufferMerge + nIn + 46, DelayBuff);
    }
};

template<class TPCM, int nIn>
const float TQmf<TPCM, nIn>::TapHalf[24] = {
    -0.00001461907,  -0.00009205479, -0.000056157569,  0.00030117269,
    0.0002422519,    -0.00085293897, -0.0005205574,    0.0020340169,
    0.00078333891,   -0.0042153862,  -0.00075614988,   0.0078402944,
    -0.000061169922, -0.01344162,    0.0024626821,     0.021736089,
    -0.007801671,    -0.034090221,   0.01880949,       0.054326009,
    -0.043596379,    -0.099384367,   0.13207909,       0.46424159
};
