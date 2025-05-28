#pragma once

#include <vector>
#include <cstdint>
#include "atrac1.h" // For TAtrac1Data, TScaledBlock

namespace NAtracDEnc {
namespace NAtrac1 {

struct TAtrac1EncoderChannelIntermediateData {
    std::vector<float> pcm_input;

    // QMF outputs
    std::vector<float> qmf_output_low; // [128]
    std::vector<float> qmf_output_mid; // [128]
    std::vector<float> qmf_output_hi;  // [256]

    // MDCT specs
    std::vector<float> mdct_specs; // [512]

    // Windowing decision for this channel
    TAtrac1Data::TBlockSizeMod effective_block_size_mod;

    // Scaled blocks (output from TScaler, input to bit allocation)
    std::vector<TScaledBlock> scaled_blocks_data;

    // Bit allocation
    std::vector<uint32_t> final_bits_per_bfu; // [num_bfus_for_frame]

    uint32_t final_bfu_amount_table_idx;

    // Quantized integer values (flat list for BFUs with bits > 0)
    std::vector<int32_t> quantized_values;

    // Quantization error (flat list, corresponding to quantized_values)
    std::vector<float> quantization_error;

    // Raw bitstream payload for this channel's frame
    std::vector<char> frame_bitstream_payload;

    TAtrac1EncoderChannelIntermediateData() {
        pcm_input.resize(TAtrac1Data::NumSamples);
        qmf_output_low.resize(128);
        qmf_output_mid.resize(128);
        qmf_output_hi.resize(256);
        mdct_specs.resize(TAtrac1Data::NumSamples);
    }

    void ClearFlatData() {
        quantized_values.clear();
        quantization_error.clear();
        frame_bitstream_payload.clear();
        final_bits_per_bfu.clear();
    }
};

struct TAtrac1EncoderFrameIntermediateData {
    std::vector<TAtrac1EncoderChannelIntermediateData> channel_data;
    uint32_t num_channels = 0;

    void resize_channels(uint32_t channels) {
        if (num_channels != channels) {
            num_channels = channels;
            channel_data.resize(channels);
        }
        // Always clear flat data for all channels before processing a new frame
        for (auto& ch_data : channel_data) {
            ch_data.ClearFlatData();
        }
    }
};

} // namespace NAtrac1
} // namespace NAtracDEnc