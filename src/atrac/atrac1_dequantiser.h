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
#include "atrac1.h"
#include "atrac_scale.h"
#include <vector>
#include <cstdint>

namespace NAtracDEnc {
namespace NAtrac1 {

class TAtrac1Dequantiser {
    std::vector<uint32_t> m_parsed_word_lengths[2];
    std::vector<uint32_t> m_parsed_scale_factor_indices[2];
    std::vector<std::vector<int32_t>> m_parsed_quantized_values[2];
    TAtrac1Data::TBlockSizeMod m_parsed_block_size_mod[2];
    std::vector<float> m_output_specs[2];

public:
    TAtrac1Dequantiser();
    void Dequant(NBitStream::TBitStream* stream, float out_specs[512], uint32_t channel_idx);

    // Getters for Python bindings
    TAtrac1Data::TBlockSizeMod GetBlockSizeMod(uint32_t channel_idx) const {
        if (channel_idx < 2) return m_parsed_block_size_mod[channel_idx];
        return {}; // Default
    }
    const std::vector<uint32_t>& get_parsed_word_lengths(uint32_t channel_idx) const {
        if (channel_idx < 2) return m_parsed_word_lengths[channel_idx];
        static std::vector<uint32_t> empty_vec; return empty_vec;
    }
    const std::vector<uint32_t>& get_parsed_scale_factor_indices(uint32_t channel_idx) const {
        if (channel_idx < 2) return m_parsed_scale_factor_indices[channel_idx];
        static std::vector<uint32_t> empty_vec; return empty_vec;
    }
    const std::vector<std::vector<int32_t>>& get_parsed_quantized_values(uint32_t channel_idx) const {
        if (channel_idx < 2) return m_parsed_quantized_values[channel_idx];
        static std::vector<std::vector<int32_t>> empty_vec_outer; return empty_vec_outer;
    }
    const std::vector<float>& get_output_specs(uint32_t channel_idx) const {
        if (channel_idx < 2) return m_output_specs[channel_idx];
        static std::vector<float> empty_specs; return empty_specs;
    }
};

} //namespace NAtrac1
} //namespace NAtracDEnc
