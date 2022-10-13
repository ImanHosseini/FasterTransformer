/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#pragma once

#include "nvToolsExt.h"
#include <iostream>

extern bool NVTX_ON;

const uint32_t colors[] = { 0xff00ff00, 0xff0000ff, 0xffffff00, 0xffff00ff, 0xff00ffff, 0xffff0000, 0xffffffff };
const int num_colors = sizeof(colors)/sizeof(uint32_t);

namespace nvtx {
static std::string scope;
std::string        getScope();
void               addScope(std::string name);
void               setScope(std::string name);
void               resetScope();
}  // namespace nvtx

#ifdef USE_NVTX

#define PUSH_RANGE(name)                                                                                               \
    {                                                                                                                  \
        if (NVTX_ON == true) {                                                                                         \
            cudaDeviceSynchronize();                                                                                   \
            nvtxRangePush((nvtx::getScope() + name).c_str());                                                          \
        }                                                                                                              \
    }

#define PUSH_RANGEC(name, cid)                                                                                         \
    {                                                                                                                  \
        if (NVTX_ON == true) {                                                                                         \
            int color_id = cid;                                                                                        \
            color_id = color_id%num_colors; \
            nvtxEventAttributes_t eventAttrib = {0}; \
            eventAttrib.version = NVTX_VERSION; \
            eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE; \
            eventAttrib.colorType = NVTX_COLOR_ARGB; \
            eventAttrib.color = colors[color_id]; \
            eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII; \
            eventAttrib.message.ascii = name;                                                                          \
            cudaDeviceSynchronize();                                                                                   \
            nvtxRangePush((nvtx::getScope() + name).c_str());                                                          \
        }                                                                                                              \
    }

#define POP_RANGE                                                                                                      \
    {                                                                                                                  \
        if (NVTX_ON == true) {                                                                                         \
            cudaDeviceSynchronize();                                                                                   \
            nvtxRangePop();                                                                                            \
        }                                                                                                              \
    }

#else

#define PUSH_RANGE(name)
#define PUSH_RANGEC(name, cid)
#define POP_RANGE

#endif
