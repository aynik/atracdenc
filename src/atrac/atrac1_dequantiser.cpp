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

#include "atrac1_dequantiser.h"
#include <string.h>
namespace NAtracDEnc {
namespace NAtrac1 {

using namespace NBitStream;

TAtrac1Dequantiser::TAtrac1Dequantiser() {
    for (int i = 0; i < 2; ++i) {
        m_output_specs[i].resize(TAtrac1Data::NumSamples, 0.0f);
    }
}

void TAtrac1Dequantiser::Dequant(TBitStream* stream, float out_specs[512], uint32_t channel_idx) {
    if (channel_idx >= 2) return;

    m_parsed_word_lengths[channel_idx].assign(TAtrac1Data::MaxBfus, 0);
    m_parsed_scale_factor_indices[channel_idx].assign(TAtrac1Data::MaxBfus, 0);
    m_parsed_quantized_values[channel_idx].assign(TAtrac1Data::MaxBfus, std::vector<int32_t>());
    std::fill(m_output_specs[channel_idx].begin(), m_output_specs[channel_idx].end(), 0.0f);
    memset(out_specs, 0, TAtrac1Data::NumSamples * sizeof(float));

    m_parsed_block_size_mod[channel_idx].LogCount[0] = (0x2 - stream->Read(2));
    m_parsed_block_size_mod[channel_idx].LogCount[1] = (0x2 - stream->Read(2));
    m_parsed_block_size_mod[channel_idx].LogCount[2] = (0x3 - stream->Read(2));
    stream->Read(2);

    const uint32_t bfuAmountIdx = stream->Read(TAtrac1Data::BitsPerBfuAmountTabIdx);
    const uint32_t numBFUsInStream = TAtrac1Data::BfuAmountTab[bfuAmountIdx];
    stream->Read(2);
    stream->Read(3);

    for (uint32_t i = 0; i < numBFUsInStream; i++) {
        m_parsed_word_lengths[channel_idx][i] = stream->Read(4);
    }
    for (uint32_t i = 0; i < numBFUsInStream; i++) {
        m_parsed_scale_factor_indices[channel_idx][i] = stream->Read(6);
    }

    for (uint32_t bandNum = 0; bandNum < TAtrac1Data::NumQMF; bandNum++) {
        for (uint32_t bfuIdxInTables = TAtrac1Data::BlocksPerBand[bandNum];
             bfuIdxInTables < TAtrac1Data::BlocksPerBand[bandNum + 1] && bfuIdxInTables < numBFUsInStream;
             bfuIdxInTables++) {

            const uint32_t bfuNum = bfuIdxInTables;
            const uint32_t numSpecs = TAtrac1Data::SpecsPerBlock[bfuNum];
            const uint32_t idwl = m_parsed_word_lengths[channel_idx][bfuNum];
            const uint32_t actual_word_len_bits = idwl ? (idwl + 1) : 0;
            const float scaleFactor = TAtrac1Data::ScaleTable[m_parsed_scale_factor_indices[channel_idx][bfuNum]];
            const uint32_t startPos = m_parsed_block_size_mod[channel_idx].LogCount[bandNum] ?
                TAtrac1Data::SpecsStartShort[bfuNum] : TAtrac1Data::SpecsStartLong[bfuNum];

            m_parsed_quantized_values[channel_idx][bfuNum].resize(numSpecs, 0);

            if (actual_word_len_bits > 0) {
                float maxQuantInverse = 0.0f;
                if (actual_word_len_bits >= 2) {
                    maxQuantInverse = 1.0f / (float)((1 << (actual_word_len_bits - 1)) - 1);
                } else {
                    maxQuantInverse = 1.0f;
                }

                for (uint32_t i = 0; i < numSpecs; i++ ) {
                    int32_t parsed_int_val = MakeSign(stream->Read(actual_word_len_bits), actual_word_len_bits);
                    m_parsed_quantized_values[channel_idx][bfuNum][i] = parsed_int_val;

                    if (actual_word_len_bits == 1) {
                        out_specs[startPos + i] = 0.0f;
                    } else {
                        out_specs[startPos + i] = scaleFactor * maxQuantInverse * static_cast<float>(parsed_int_val);
                    }
                }
            } else {
                m_parsed_quantized_values[channel_idx][bfuNum].clear();
            }
        }
    }
    std::copy(out_specs, out_specs + TAtrac1Data::NumSamples, m_output_specs[channel_idx].begin());
}

} //namespace NAtrac1
} //namespace NAtracDEnc
