#pragma once
#include <iostream>
#include <vector>

#include "device.hpp"
#include "device_base.hpp"
#include "gridwise_binary_elementwise_1d.hpp"

namespace ck {
namespace tensor_operation {
namespace device {

template <typename ADataType,
          typename BDataType,
          typename CDataType,
          typename ComputeDataType,
          typename ElementwiseFunctor,
          index_t Dim,
          index_t M0PerThread,
          index_t AScalarPerVector,
          index_t BScalarPerVector,
          index_t CScalarPerVector>
struct DeviceBinaryElementwise : public BaseOperator
{
    static constexpr auto I0 = Number<0>{};

    template <typename Desc_M0>
    static auto PadDescriptor_M0_1d(Desc_M0 desc_m0, index_t gridSize, index_t blockSize)
    {
        const auto m0           = desc_m0.GetLength(I0);
        const index_t loop_step = gridSize * blockSize * M0PerThread;
        const auto pad          = math::integer_least_multiple(m0, loop_step) - m0;
        const auto desc_m0_pad =
            transform_tensor_descriptor(desc_m0,
                                        make_tuple(make_right_pad_transform(m0, pad)),
                                        make_tuple(Sequence<0>{}),
                                        make_tuple(Sequence<0>{}));
        return desc_m0_pad;
    }

    static auto MakeDescriptor_M0(const std::vector<index_t>& lengths,
                                  const std::vector<index_t>& strides,
                                  index_t gridSize,
                                  index_t blockSize)
    {
        auto tupleOfShape  = generate_tuple([&](auto I) { return lengths[I]; }, Number<Dim>{});
        auto tupleOfStride = generate_tuple([&](auto I) { return strides[I]; }, Number<Dim>{});

        // nd desc - [s0, s1, s2, ...]
        const auto desc = make_naive_tensor_descriptor(tupleOfShape, tupleOfStride);

        // merge nd to 1d desc - [s0 * s1 * ...]
        if constexpr(Dim > 1)
        {
            const auto desc_m0 = transform_tensor_descriptor(
                desc,
                make_tuple(make_merge_transform(tupleOfShape)),
                make_tuple(generate_sequence_v2([&](auto I) { return I; }, Number<Dim>{})),
                make_tuple(Sequence<0>{}));

            return PadDescriptor_M0_1d(desc_m0, gridSize, blockSize);
        }
        else
            return PadDescriptor_M0_1d(desc, gridSize, blockSize);
    }

    using GridDesc_M0        = decltype(MakeDescriptor_M0({1, 1}, {1, 1}, 1, 1));
    using GridwiseBinEltwise = GridwiseBinaryElementwise_1D<ADataType,
                                                            BDataType,
                                                            CDataType,
                                                            ComputeDataType,
                                                            GridDesc_M0,
                                                            ElementwiseFunctor,
                                                            M0PerThread,
                                                            AScalarPerVector,
                                                            BScalarPerVector,
                                                            CScalarPerVector>;

    struct Argument : public BaseArgument
    {
        Argument(const ADataType* p_a,
                 const BDataType* p_b,
                 CDataType* p_c,
                 const std::vector<index_t>& lengths,
                 const std::vector<index_t>& a_strides,
                 const std::vector<index_t>& b_strides,
                 const std::vector<index_t>& c_strides,
                 ElementwiseFunctor functor)
            : p_a_(p_a),
              p_b_(p_b),
              p_c_(p_c),
              lengths_(lengths),
              a_strides_(a_strides),
              b_strides_(b_strides),
              c_strides_(c_strides),
              functor_(functor),
              blockSize_(256),
              gridSize_(120) // FIXME - Calculate the grid size by number of CU in the future
        {
            a_grid_desc_m0_ = MakeDescriptor_M0(lengths, a_strides, gridSize_, blockSize_);
            b_grid_desc_m0_ = MakeDescriptor_M0(lengths, b_strides, gridSize_, blockSize_);
            c_grid_desc_m0_ = MakeDescriptor_M0(lengths, c_strides, gridSize_, blockSize_);
        }

        const ADataType* p_a_;
        const BDataType* p_b_;
        CDataType* p_c_;
        std::vector<int> lengths_;
        GridDesc_M0 a_grid_desc_m0_;
        GridDesc_M0 b_grid_desc_m0_;
        GridDesc_M0 c_grid_desc_m0_;
        std::vector<index_t> a_strides_;
        std::vector<index_t> b_strides_;
        std::vector<index_t> c_strides_;
        ElementwiseFunctor functor_;
        index_t blockSize_;
        index_t gridSize_;
    };

    struct Invoker : public BaseInvoker
    {
        float Run(const Argument& arg, const StreamConfig& stream_config = StreamConfig{})
        {
            const auto kernel = kernel_binary_elementwise_1d<GridwiseBinEltwise,
                                                             ADataType,
                                                             BDataType,
                                                             CDataType,
                                                             GridDesc_M0,
                                                             ElementwiseFunctor>;

            float elapsed_time = launch_and_time_kernel(stream_config,
                                                        kernel,
                                                        dim3(arg.gridSize_),
                                                        dim3(arg.blockSize_),
                                                        0,
                                                        arg.p_a_,
                                                        arg.p_b_,
                                                        arg.p_c_,
                                                        arg.a_grid_desc_m0_,
                                                        arg.b_grid_desc_m0_,
                                                        arg.c_grid_desc_m0_,
                                                        arg.functor_);
            return elapsed_time;
        }

        // polymorphic
        float Run(const BaseArgument* p_arg,
                  const StreamConfig& stream_config = StreamConfig{}) override
        {
            return Run(*dynamic_cast<const Argument*>(p_arg), stream_config);
        }
    };

    bool IsSupportedArgument(const BaseArgument* p_arg) override
    {
        const Argument* pArg = dynamic_cast<const Argument*>(p_arg);

        if(pArg == nullptr)
            return false;

        if(pArg->lengths_.size() != Dim)
            return false;

        if(pArg->lengths_.back() % M0PerThread != 0)
            return false;

        auto IsScalarPerVectorValid = [](bool isFastestAxisCoalesce, int scalarPerVector) {
            bool ret = true;

            if(!isFastestAxisCoalesce)
                ret = scalarPerVector == 1;
            else
                ret = M0PerThread % scalarPerVector == 0;

            return ret;
        };

        if(!IsScalarPerVectorValid(pArg->a_strides_.back() == 1, AScalarPerVector))
            return false;

        if(!IsScalarPerVectorValid(pArg->b_strides_.back() == 1, BScalarPerVector))
            return false;

        if(!IsScalarPerVectorValid(pArg->c_strides_.back() == 1, CScalarPerVector))
            return false;

        return true;
    };

    std::unique_ptr<BaseArgument> MakeArgumentPointer(const void* p_a,
                                                      const void* p_b,
                                                      void* p_c,
                                                      std::vector<index_t> lengths,
                                                      std::vector<index_t> a_strides,
                                                      std::vector<index_t> b_strides,
                                                      std::vector<index_t> c_strides,
                                                      ElementwiseFunctor functor)
    {
        return std::make_unique<Argument>(static_cast<const ADataType*>(p_a),
                                          static_cast<const BDataType*>(p_b),
                                          static_cast<CDataType*>(p_c),
                                          lengths,
                                          a_strides,
                                          b_strides,
                                          c_strides,
                                          functor);
    }

    std::unique_ptr<BaseInvoker> MakeInvokerPointer() { return std::make_unique<Invoker>(); }

    std::string GetTypeString() const override
    {
        auto str = std::stringstream();

        // clang-format off
        str << "DeviceBinaryElementwise"
            << "<"
            << "M0PerThread = " << M0PerThread
            << ">";
        // clang-format on

        return str.str();
    }
};

} // namespace device
} // namespace tensor_operation
} // namespace ck
