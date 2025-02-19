/*
 * Copyright (c) 2019-2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/fastertransformer/models/gptj_int8/GptJDecoderLayerWeightINT8.h"
#include "src/fastertransformer/utils/memory_utils.h"

namespace fastertransformer {

template<typename T>
GptJDecoderLayerWeightINT8<T>::GptJDecoderLayerWeightINT8(const int hidden_units,
                                                  const int inter_size,
                                                  const int tensor_para_size,
                                                  const int tensor_para_rank):
    hidden_units_(hidden_units),
    inter_size_(inter_size),
    tensor_para_size_(tensor_para_size),
    tensor_para_rank_(tensor_para_rank)
{
    mallocWeights();
    setWeightPtr();
}

template<typename T>
GptJDecoderLayerWeightINT8<T>::~GptJDecoderLayerWeightINT8()
{
    if (is_maintain_buffer == true) {
        for (int i = 0; i < 9; i++) {
            if(i == 2 || i == 3) continue;
            deviceFree(weights_ptr[i]);
        }

        pre_layernorm_weights.beta                            = nullptr;
        pre_layernorm_weights.gamma                           = nullptr;
        self_attention_weights.query_weight.kernel            = nullptr;
        self_attention_weights.query_weight.bias              = nullptr;
        self_attention_weights.attention_output_weight.kernel = nullptr;

        ffn_weights.intermediate_weight.kernel = nullptr;
        ffn_weights.intermediate_weight.bias   = nullptr;
        ffn_weights.output_weight.kernel       = nullptr;
        ffn_weights.output_weight.bias         = nullptr;
        is_maintain_buffer                     = false;
    }
}

template<typename T>
GptJDecoderLayerWeightINT8<T>::GptJDecoderLayerWeightINT8(const GptJDecoderLayerWeightINT8& other):
    hidden_units_(other.hidden_units_),
    inter_size_(other.inter_size_),
    tensor_para_size_(other.tensor_para_size_),
    tensor_para_rank_(other.tensor_para_rank_)
{
    mallocWeights();

    cudaD2Dcpy(weights_ptr[0], other.weights_ptr[0], hidden_units_);
    cudaD2Dcpy(weights_ptr[1], other.weights_ptr[1], hidden_units_);
    // cudaD2Dcpy(weights_ptr[2], other.weights_ptr[2], hidden_units_ * 3 * hidden_units_ / tensor_para_size_);
    // cudaD2Dcpy(qwint8, other.qwint8, hidden_units_ * 3 * hidden_units_ / tensor_para_size_);
    // cudaD2Dcpy(qwscale, other.qwscale, 3 * hidden_units_ / tensor_para_size_);
    // cudaD2Dcpy(weights_ptr[3], other.weights_ptr[3], 3 * hidden_units_ / tensor_para_size_);
    cudaD2Dcpy(weights_ptr[4], other.weights_ptr[4], hidden_units_ / tensor_para_size_ * hidden_units_);

    cudaD2Dcpy(weights_ptr[5], other.weights_ptr[5], hidden_units_ * inter_size_ / tensor_para_size_);
    cudaD2Dcpy(weights_ptr[6], other.weights_ptr[6], inter_size_ / tensor_para_size_);
    cudaD2Dcpy(weights_ptr[7], other.weights_ptr[7], inter_size_ / tensor_para_size_ * hidden_units_);
    cudaD2Dcpy(weights_ptr[8], other.weights_ptr[8], hidden_units_);

    setWeightPtr();
}

template<typename T>
GptJDecoderLayerWeightINT8<T>& GptJDecoderLayerWeightINT8<T>::operator=(const GptJDecoderLayerWeightINT8& other)
{
    hidden_units_     = other.hidden_units_;
    inter_size_       = other.inter_size_;
    tensor_para_size_ = other.tensor_para_size_;
    tensor_para_rank_ = other.tensor_para_rank_;

    mallocWeights();

    cudaD2Dcpy(weights_ptr[0], other.weights_ptr[0], hidden_units_);
    cudaD2Dcpy(weights_ptr[1], other.weights_ptr[1], hidden_units_);
    // cudaD2Dcpy(weights_ptr[2], other.weights_ptr[2], hidden_units_ * 3 * hidden_units_ / tensor_para_size_);
    // cudaD2Dcpy(qwint8, other.qwint8, hidden_units_ * 3 * hidden_units_ / tensor_para_size_);
    // cudaD2Dcpy(qwscale, other.qwscale, 3 * hidden_units_ / tensor_para_size_);
    // cudaD2Dcpy(weights_ptr[3], other.weights_ptr[3], 3 * hidden_units_ / tensor_para_size_);
    cudaD2Dcpy(weights_ptr[4], other.weights_ptr[4], hidden_units_ / tensor_para_size_ * hidden_units_);

    cudaD2Dcpy(weights_ptr[5], other.weights_ptr[5], hidden_units_ * inter_size_ / tensor_para_size_);
    cudaD2Dcpy(weights_ptr[6], other.weights_ptr[6], inter_size_ / tensor_para_size_);
    cudaD2Dcpy(weights_ptr[7], other.weights_ptr[7], inter_size_ / tensor_para_size_ * hidden_units_);
    cudaD2Dcpy(weights_ptr[8], other.weights_ptr[8], hidden_units_);

    setWeightPtr();
    return *this;
}

template<typename T>
void GptJDecoderLayerWeightINT8<T>::loadModel(std::string dir_path, FtCudaDataType model_file_type)
{
    // FT_LOG_INFO("GPTJ-X");
    FT_CHECK(is_maintain_buffer == true);
    const std::string rank_spec = std::to_string(tensor_para_rank_);

    loadWeightFromBin<T>(
        weights_ptr[0], {(size_t)hidden_units_}, dir_path + ".input_layernorm.bias.bin", model_file_type);
    loadWeightFromBin<T>(
        weights_ptr[1], {(size_t)hidden_units_}, dir_path + ".input_layernorm.weight.bin", model_file_type);
    // load & quantize
    // [DBG]
    //  loadWeightFromBinQ<T>(weights_ptr[2],
    //                      {(size_t)hidden_units_, (size_t)(3 * hidden_units_ / tensor_para_size_)},
    //                      dir_path + ".attention.query_key_value.weight." + rank_spec + ".bin",
    //                      model_file_type);
    // exit(0);
    // [END DBG]
    // loadWeightFromBin<T>(weights_ptr[2],
    //                      {(size_t)hidden_units_, (size_t)(3 * hidden_units_ / tensor_para_size_)},
    //                      dir_path + ".attention.query_key_value.weight." + rank_spec + ".bin",
    //                      model_file_type);
    loadWeightFromBinQ<T>(self_attention_weights.query_weight, {(size_t)hidden_units_, (size_t)(3 * hidden_units_ / tensor_para_size_)}, 
                          dir_path + ".attention.query_key_value.weight." + rank_spec + ".bin", model_file_type);
    // GPT-J does not have bias for QKV
    // Why are we still allocating a buffer for bias when we don't use it?
    // cudaMemset(weights_ptr[3], 0, sizeof(T) * 3 * hidden_units_ / tensor_para_size_);
    loadWeightFromBin<T>(weights_ptr[4],
                         {(size_t)(hidden_units_ / tensor_para_size_), (size_t)hidden_units_},
                         dir_path + ".attention.dense.weight." + rank_spec + ".bin",
                         model_file_type);

    loadWeightFromBin<T>(weights_ptr[5],
                         {(size_t)hidden_units_, (size_t)(inter_size_ / tensor_para_size_)},
                         dir_path + ".mlp.dense_h_to_4h.weight." + rank_spec + ".bin",
                         model_file_type);
    loadWeightFromBin<T>(weights_ptr[6],
                         {(size_t)(inter_size_ / tensor_para_size_)},
                         dir_path + ".mlp.dense_h_to_4h.bias." + rank_spec + ".bin",
                         model_file_type);
    loadWeightFromBin<T>(weights_ptr[7],
                         {(size_t)(inter_size_ / tensor_para_size_), (size_t)hidden_units_},
                         dir_path + ".mlp.dense_4h_to_h.weight." + rank_spec + ".bin",
                         model_file_type);
    loadWeightFromBin<T>(
        weights_ptr[8], {(size_t)hidden_units_}, dir_path + ".mlp.dense_4h_to_h.bias.bin", model_file_type);
}

template<typename T>
void GptJDecoderLayerWeightINT8<T>::setWeightPtr()
{
    pre_layernorm_weights.beta                            = weights_ptr[0];
    pre_layernorm_weights.gamma                           = weights_ptr[1];
    // instead set attention_output_weight.int8_kernel and scale

    // self_attention_weights.query_weight.kernel            = weights_ptr[2];
    // self_attention_weights.query_weight.int8_kernel = qwint8;
    // self_attention_weights.query_weight.scale = qwscale;
    // self_attention_weights.query_weight.bias              = weights_ptr[3];
    self_attention_weights.attention_output_weight.kernel = weights_ptr[4];

    ffn_weights.intermediate_weight.kernel = weights_ptr[5];
    ffn_weights.intermediate_weight.bias   = weights_ptr[6];
    ffn_weights.output_weight.kernel       = weights_ptr[7];
    ffn_weights.output_weight.bias         = weights_ptr[8];

    is_maintain_buffer = true;
}

template<typename T>
void GptJDecoderLayerWeightINT8<T>::mallocWeights()
{
    deviceMalloc(&weights_ptr[0], hidden_units_);
    deviceMalloc(&weights_ptr[1], hidden_units_);
    // deviceMalloc(&weights_ptr[2], hidden_units_ * 3 * hidden_units_ / tensor_para_size_);
    // deviceMalloc(&weights_ptr[3], 3 * hidden_units_ / tensor_para_size_);
    // deviceMalloc(&qwint8, hidden_units_ * 3 * hidden_units_ / tensor_para_size_);
    // deviceMalloc(&qwscale, 3 * hidden_units_ / tensor_para_size_);
    deviceMalloc(&weights_ptr[4], hidden_units_ / tensor_para_size_ * hidden_units_);

    deviceMalloc(&weights_ptr[5], hidden_units_ * inter_size_ / tensor_para_size_);
    deviceMalloc(&weights_ptr[6], inter_size_ / tensor_para_size_);
    deviceMalloc(&weights_ptr[7], inter_size_ / tensor_para_size_ * hidden_units_);
    deviceMalloc(&weights_ptr[8], hidden_units_);
}

template struct GptJDecoderLayerWeightINT8<float>;
template struct GptJDecoderLayerWeightINT8<half>;
// template struct GptJDecoderLayerWeightINT8<int8_t>;
#ifdef ENABLE_BF16
template struct GptJDecoderLayerWeightINT8<__nv_bfloat16>;
#endif

}  // namespace fastertransformer
