// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022, Advanced Micro Devices, Inc. All rights reserved.

#include <stdlib.h>
#include "config.hpp"
#include "device_gemm_xdl_c_shuffle_bias_activation.hpp"
#include "element_wise_operation.hpp"
#include "device_operation_instance.hpp"

namespace ck {
namespace tensor_operation {
namespace device {
namespace device_gemm_instance {

using F16 = ck::half_t;
using F32 = float;

using Row = ck::tensor_layout::gemm::RowMajor;
using Col = ck::tensor_layout::gemm::ColumnMajor;

template <ck::index_t... Is>
using S = ck::Sequence<Is...>;

using PassThrough = ck::tensor_operation::element_wise::PassThrough;
using AddRelu     = ck::tensor_operation::element_wise::AddRelu;

// c[m, n] = ReLU(a[k, m] * b[k, n] + c0[n])
using device_gemm_xdl_c_shuffle_bias_relu_f16_f16_f16_km_kn_mn_instances = std::tuple<
    // clang-format off
        //#####################################|AData| BData| CData| AccData| ALayout| BLayout| CLayout|           A|           B|           C| Block|  MPer|  NPer| K0Per| K1| MPer| NPer| MXdl| NXdl|  ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockLds|  BBlockTransfer| BBlockTransfer| BBlockTransfer| BlockTransfer| BBlockTransfer| BBlockTransfer| BBlockLds|    CShuffle|    CShuffle|     CBlockTransferClusterLengths|  CBlockTransfer|
        //#####################################| Type|  Type|  Type|    Type|        |        |        | Elementwise| Elementwise| Elementwise|  Size| Block| Block| Block|   |  XDL|  XDL|  Per|  Per|   ThreadCluster|  ThreadCluster| SrcAccessOrder|   SrcVectorDim|      SrcScalar|      DstScalar| AddExtraM|   ThreadCluster|  ThreadCluster| SrcAccessOrder|  SrcVectorDim|      SrcScalar|      DstScalar| AddExtraN| MXdlPerWave| NXdlPerWave| _MBlock_MXdlPerWave_MWaveMPerXdl| ScalarPerVector|
        //#####################################|     |      |      |        |        |        |        |   Operation|   Operation|   Operation|      |      |      |      |   |     |     | Wave| Wave| Lengths_K0_M_K1|   ArrangeOrder|               |               |      PerVector|   PerVector_K1|          | Lengths_K0_N_K1|   ArrangeOrder|               |              |      PerVector|   PerVector_K1|          |  PerShuffle|  PerShuffle| _NBlock_NXdlPerWave_NWaveNPerXdl|   _NWaveNPerXdl|
        //#####################################|     |      |      |        |        |        |        |            |            |            |      |      |      |      |   |     |     |     |     |                |               |               |               |               |               |          |                |               |               |              |               |               |          |            |            |                                 |                |
        DeviceGemmXdl_C_Shuffle_Bias_Activation<  F16,   F16,   F16,     F32,     Col,      Row,    Row, PassThrough, PassThrough,     AddRelu,   256,   256,   128,     4,  8,   32,   32,    4,    2,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              8,      true,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              2,              8,      true,           1,           1,             S<1, 1, 32, 1, 1, 8>,               8>,
        DeviceGemmXdl_C_Shuffle_Bias_Activation<  F16,   F16,   F16,     F32,     Col,      Row,    Row, PassThrough, PassThrough,     AddRelu,   256,   128,   256,     4,  8,   32,   32,    2,    4,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              2,              8,      true,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              4,              8,      true,           1,           1,             S<1, 1, 32, 1, 1, 8>,               8>,
        DeviceGemmXdl_C_Shuffle_Bias_Activation<  F16,   F16,   F16,     F32,     Col,      Row,    Row, PassThrough, PassThrough,     AddRelu,   128,   128,   128,     4,  8,   32,   32,    4,    2,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              8,      true,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              4,              8,      true,           1,           1,             S<1, 1, 16, 1, 1, 8>,               8>,
        DeviceGemmXdl_C_Shuffle_Bias_Activation<  F16,   F16,   F16,     F32,     Col,      Row,    Row, PassThrough, PassThrough,     AddRelu,   256,   128,   128,     4,  8,   32,   32,    2,    2,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              2,              8,      true,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              2,              8,      true,           1,           1,             S<1, 1, 32, 1, 1, 8>,               8>,
        DeviceGemmXdl_C_Shuffle_Bias_Activation<  F16,   F16,   F16,     F32,     Col,      Row,    Row, PassThrough, PassThrough,     AddRelu,   128,   128,    64,     4,  8,   32,   32,    2,    2,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              8,      true,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              2,              8,      true,           1,           1,             S<1, 1, 32, 1, 1, 4>,               8>,
        DeviceGemmXdl_C_Shuffle_Bias_Activation<  F16,   F16,   F16,     F32,     Col,      Row,    Row, PassThrough, PassThrough,     AddRelu,   128,    64,   128,     4,  8,   32,   32,    2,    2,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              2,              8,      true,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              4,              8,      true,           1,           1,             S<1, 1, 16, 1, 1, 8>,               8>,
        DeviceGemmXdl_C_Shuffle_Bias_Activation<  F16,   F16,   F16,     F32,     Col,      Row,    Row, PassThrough, PassThrough,     AddRelu,   256,   128,    64,     4,  8,   32,   32,    2,    1,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              2,              8,      true,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              1,              8,      true,           1,           1,             S<1, 1, 16, 1, 1, 4>,               8>,
        DeviceGemmXdl_C_Shuffle_Bias_Activation<  F16,   F16,   F16,     F32,     Col,      Row,    Row, PassThrough, PassThrough,     AddRelu,   256,    64,   128,     4,  8,   32,   32,    1,    2,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              1,              8,      true,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              2,              8,      true,           1,           1,             S<1, 1, 32, 1, 1, 8>,               8>
    // clang-format on
    >;

void add_device_gemm_xdl_c_shuffle_bias_relu_f16_f16_f16_km_kn_mn_instances(
    std::vector<DeviceGemmBiasActivationPtr<PassThrough, PassThrough, AddRelu>>& instances)
{
    add_device_operation_instances(
        instances, device_gemm_xdl_c_shuffle_bias_relu_f16_f16_f16_km_kn_mn_instances{});
}

} // namespace device_gemm_instance
} // namespace device
} // namespace tensor_operation
} // namespace ck
