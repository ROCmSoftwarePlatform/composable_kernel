#pragma once
#include <iostream>
#include <vector>

#include "device_base.hpp"

namespace ck {
namespace tensor_operation {
namespace device {

struct BatchedGemmCPermuteDesc
{
    ck::index_t G0_, G1_, M_, N_;
    ck::index_t stride_G0_, stride_G1_, stride_M_, stride_N_;
};

template <typename ALayout,
          typename BLayout,
          typename DELayout,
          typename ADataType,
          typename BDataType,
          typename EDataType,
          typename AElementwiseOperation,
          typename BElementwiseOperation,
          typename CDEElementwiseOperation>
struct DeviceBatchedGemmCPermute : public BaseOperator
{
    virtual std::unique_ptr<BaseArgument>
    MakeArgumentPointer(const void* p_a,
                        const void* p_b,
                        void* p_c,
                        index_t M,
                        index_t N,
                        index_t K,
                        index_t stride_A,
                        index_t stride_B,
                        index_t batch_stride_A,
                        index_t batch_stride_B,
                        BatchedGemmCPermuteDesc batched_gemm_c_permute_desc,
                        index_t BatchCount,
                        AElementwiseOperation a_element_op,
                        BElementwiseOperation b_element_op,
                        CDEElementwiseOperation c_element_op) = 0;

    virtual std::unique_ptr<BaseInvoker> MakeInvokerPointer() = 0;
};

template <typename ALayout,
          typename BLayout,
          typename DELayout,
          typename ADataType,
          typename BDataType,
          typename EDataType,
          typename AElementwiseOperation,
          typename BElementwiseOperation,
          typename CDEElementwiseOperation>
using DeviceBatchedGemmCPermutePtr =
    std::unique_ptr<DeviceBatchedGemmCPermute<ALayout,
                                              BLayout,
                                              DELayout,
                                              ADataType,
                                              BDataType,
                                              EDataType,
                                              AElementwiseOperation,
                                              BElementwiseOperation,
                                              CDEElementwiseOperation>>;

} // namespace device
} // namespace tensor_operation
} // namespace ck
