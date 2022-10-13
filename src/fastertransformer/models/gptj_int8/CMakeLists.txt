# Copyright (c) 2019-2022, NVIDIA CORPORATION.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.8)

add_library(GptJDecoderLayerWeightINT8 STATIC GptJDecoderLayerWeightINT8.cc)
set_property(TARGET GptJDecoderLayerWeightINT8 PROPERTY POSITION_INDEPENDENT_CODE  ON)
set_property(TARGET GptJDecoderLayerWeightINT8 PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS  ON)
target_link_libraries(GptJDecoderLayerWeightINT8 PUBLIC memory_utils)

add_library(GptJDecoderINT8 STATIC GptJDecoderINT8.cc)
set_property(TARGET GptJDecoderINT8 PROPERTY POSITION_INDEPENDENT_CODE  ON)
set_property(TARGET GptJDecoderINT8 PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS  ON)
target_link_libraries(GptJDecoderINT8 PUBLIC -lcudart cublasMMWrapper cublasINT8MMWrapper
                      TensorParallelDecoderSelfAttentionLayer
                      TensorParallelGeluFfnLayer
                      layernorm_kernels
                      layernorm_int8_kernels
                      add_residual_kernels
                      GptJDecoderLayerWeightINT8
                      tensor
                      nccl_utils)

add_library(GptJContextDecoderINT8 STATIC GptJContextDecoderINT8.cc)
set_property(TARGET GptJContextDecoderINT8 PROPERTY POSITION_INDEPENDENT_CODE  ON)
set_property(TARGET GptJContextDecoderINT8 PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS  ON)
target_link_libraries(GptJContextDecoderINT8 PUBLIC -lcudart cublasMMWrapper cublasINT8MMWrapper
                      TensorParallelGptContextAttentionLayer
                      TensorParallelGeluFfnLayer
                      layernorm_kernels
                      layernorm_int8_kernels
                      add_residual_kernels
                      gpt_kernels
                      tensor
                      nccl_utils)

add_library(GptJWeightINT8 STATIC GptJWeightINT8.cc)
set_property(TARGET GptJWeightINT8 PROPERTY POSITION_INDEPENDENT_CODE  ON)
set_property(TARGET GptJWeightINT8 PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS  ON)
target_link_libraries(GptJWeightINT8 PUBLIC GptJDecoderLayerWeightINT8)

add_library(GptJINT8 STATIC GptJINT8.cc)
set_property(TARGET GptJINT8 PROPERTY POSITION_INDEPENDENT_CODE  ON)
set_property(TARGET GptJINT8 PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS  ON)
target_link_libraries(GptJINT8 PUBLIC -lcudart
                      GptJDecoderINT8
                      GptJContextDecoderINT8
                      decoding_kernels
                      gpt_kernels
                      DynamicDecodeLayer
                      BaseBeamSearchLayer
                      bert_preprocess_kernels
                      tensor
                      GptJWeightINT8)