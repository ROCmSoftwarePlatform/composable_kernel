// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022, Advanced Micro Devices, Inc. All rights reserved.

#ifndef DEVICE_REDUCE_INSTANCE_BLOCKWISE_F16_F32_F16_HPP
#define DEVICE_REDUCE_INSTANCE_BLOCKWISE_F16_F32_F16_HPP

#include "data_type.hpp"
#include "device_reduce_instance_blockwise.hpp"

namespace ck {
namespace tensor_operation {
namespace device {
namespace device_reduce_instance {

// clang-format off
// InDataType | AccDataType | OutDataType | ReduceOpId | NanPropaOpt | IndicesOpt | Rank | NumReduceDim 
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 0, 0, 0, 4, 3); // for ADD
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 0, 0, 0, 4, 4);
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 0, 0, 0, 4, 1);
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 0, 0, 0, 2, 1);
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 5, 0, 0, 4, 3); // for AVG
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 5, 0, 0, 4, 4);       
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 5, 0, 0, 4, 1);       
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 5, 0, 0, 2, 1);       
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 7, 0, 0, 4, 3); // for NORM2
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 7, 0, 0, 4, 4);       
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 7, 0, 0, 4, 1);       
ADD_BLOCKWISE_INST_REF_BY_ID(half_t, float, half_t, 7, 0, 0, 2, 1);
// clang-format on

} // namespace device_reduce_instance
} // namespace device
} // namespace tensor_operation

} // namespace ck

#endif
