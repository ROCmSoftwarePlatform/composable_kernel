#pragma once

#include "common_header.hpp"
#include "multi_index_transform_helper.hpp"
#include "tensor_descriptor.hpp"
#include "tensor_descriptor_helper.hpp"
#include "blockwise_gemm_xdlops.hpp"
#include "blockwise_gemm_xdlops_skip_b_lds.hpp"
#include "thread_group_tensor_slice_transfer_v4r1.hpp"
#include "threadwise_tensor_slice_transfer.hpp"
#include "gridwise_gemm_pipeline_v1.hpp"

namespace ck {

template <index_t BlockSize,
          typename FloatAB,
          typename FloatAcc,
          typename FloatC,
          InMemoryDataOperationEnum CGlobalMemoryDataOperation,
          typename AGridDesc_K0_M_K1,
          typename B0GridDesc_K0_N_K1,
          typename B1GridDesc_K0_N_K1,
          typename CGridDesc_M_N,
          typename AElementwiseOperation,
          typename B0ElementwiseOperation,
          typename C0ElementwiseOperation,
          typename B1ElementwiseOperation,
          typename C1ElementwiseOperation,
          index_t NumGemmKPrefetchStage,
          index_t M0PerBlock,
          index_t N0PerBlock,
          index_t M0PerXDL,
          index_t N0PerXDL,
          index_t M1PerBlock,
          index_t N1PerBlock,
          index_t M1PerXDL,
          index_t N1PerXDL,
          index_t KPerBlock,
          index_t AK1,
          index_t B0K1,
          index_t B1K1,
          index_t M0XdlPerWave,
          index_t N0XdlPerWave,
          index_t M1XdlPerWave,
          index_t N1XdlPerWave,
          typename ABlockTransferThreadClusterLengths_K0_M_K1,
          typename ABlockTransferThreadClusterArrangeOrder,
          typename ABlockTransferSrcAccessOrder,
          index_t ABlockTransferSrcVectorDim,
          index_t ABlockTransferSrcScalarPerVector,
          index_t ABlockTransferDstScalarPerVector_K1,
          bool AThreadTransferSrcResetCoordinateAfterRun,
          bool ABlockLdsExtraM,
          typename B1BlockTransferThreadClusterLengths_K0_M_K1,
          typename B1BlockTransferThreadClusterArrangeOrder,
          typename B1BlockTransferSrcAccessOrder,
          index_t B1BlockTransferSrcVectorDim,
          index_t B1BlockTransferSrcScalarPerVector,
          index_t B1BlockTransferDstScalarPerVector_K1,
          bool B1ThreadTransferSrcResetCoordinateAfterRun,
          bool B1BlockLdsExtraM,
          index_t B0BlockTransferSrcScalarPerVector,
          bool B0ThreadTransferSrcResetCoordinateAfterRun,
          typename CThreadTransferSrcDstAccessOrder,
          index_t CThreadTransferSrcDstVectorDim,
          index_t CThreadTransferDstScalarPerVector>
struct GridwiseGemmGemmXdlopsSkipLdsV1
{
    static constexpr auto I0 = Number<0>{};
    static constexpr auto I1 = Number<1>{};
    static constexpr auto I2 = Number<2>{};
    static constexpr auto I3 = Number<3>{};
    static constexpr auto I4 = Number<4>{};
    static constexpr auto I5 = Number<5>{};
    static constexpr auto I6 = Number<6>{};
    static constexpr auto I7 = Number<7>{};

    static constexpr auto K0PerBlock = Number<KPerBlock / AK1>{};

    static constexpr auto BaseMultK0 = 2;
    static constexpr auto MultiK0    = BaseMultK0 * 1;

    // K1 should be Number<...>
    static constexpr auto K1 = Number<AK1>{};

    // gemm1 K1
    static constexpr auto AccK1 = I4;
    static constexpr auto Gemm1K0PerBlock = Number<KPerBlock / AccK1>{};

    static constexpr index_t WaveSize = 64;
    static constexpr index_t M0Waves   = M0PerBlock / (M0XdlPerWave * M0PerXDL);
    static constexpr index_t N0Waves   = N0PerBlock / (N0XdlPerWave * N0PerXDL);

    static constexpr auto xdlops_gemm0    = XdlopsGemm<FloatAB, M0PerXDL, N0PerXDL, K1>{};
    static constexpr index_t K0PerThread0 = K0PerBlock / xdlops_gemm0.K0PerXdlops;

    static constexpr index_t M1Waves   = M1PerBlock / (M1XdlPerWave * M1PerXDL);
    static constexpr index_t N1Waves   = N1PerBlock / (N1XdlPerWave * N1PerXDL);

    static constexpr auto xdlops_gemm1    = XdlopsGemm<FloatAB, M1PerXDL, N1PerXDL, K1>{};
    static constexpr index_t K0PerThread1 = K0PerBlock / xdlops_gemm1.K0PerXdlops;

    using ThisThreadBlock = ThisThreadBlock<BlockSize>;

    using GridwiseGemmPipe0 = GridwiseGemmPipelineSkipBLds;
    using GridwiseGemmPipe1 = GridwiseGemmPipelineAInVgpr;

    __host__ __device__ static constexpr auto GetABlockDescriptor_K0PerBlock_MPerBlock_K1()
    {
        constexpr auto max_lds_align = AK1;

        // A matrix in LDS memory, dst of blockwise copy
        constexpr auto a_block_desc_k0_m_k1 = [&]() {
            if constexpr(ABlockLdsExtraM)
            {
                return make_naive_tensor_descriptor(
                    make_tuple(Number<K0PerBlock * MultiK0>{}, Number<M0PerBlock>{}, K1),
                    make_tuple(Number<M0PerBlock + 1>{} * K1, K1, I1));
            }
            else
            {
                return make_naive_tensor_descriptor_aligned(
                    make_tuple(Number<K0PerBlock * MultiK0>{}, Number<M0PerBlock>{}, K1),
                    max_lds_align);
            }
        }();

        return a_block_desc_k0_m_k1;
    }

    __host__ __device__ static constexpr auto GetB1BlockDescriptor_K0PerBlock_NPerBlock_K1()
    {
        constexpr auto max_lds_align = B1K1;

        // A matrix in LDS memory, dst of blockwise copy
        constexpr auto b1_block_desc_k0_n_k1 = [&]() {
            if constexpr(ABlockLdsExtraM)
            {
                return make_naive_tensor_descriptor(
                    make_tuple(Number<K0PerBlock * MultiK0>{}, Number<N1PerBlock>{}, K1),
                    make_tuple(Number<N1PerBlock + 1>{} * K1, K1, I1));
            }
            else
            {
                return make_naive_tensor_descriptor_aligned(
                    make_tuple(Number<K0PerBlock * MultiK0>{}, Number<N1PerBlock>{}, K1),
                    max_lds_align);
            }
        }();

        return b1_block_desc_k0_n_k1;
    }

    __host__ __device__ static constexpr index_t GetSharedMemoryNumberOfByte()
    {
        // LDS allocation for A and B: be careful of alignment
        constexpr auto a_block_desc_k0_m_k1 = GetABlockDescriptor_K0PerBlock_MPerBlock_K1();
        constexpr auto b1_block_desc_k0_n_k1 = GetB1BlockDescriptor_K0PerBlock_NPerBlock_K1();

        constexpr auto max_lds_align = K1;

        constexpr auto a_block_space_size_aligned =
            math::integer_least_multiple(a_block_desc_k0_m_k1.GetElementSpaceSize(), max_lds_align);

        return (a_block_space_size_aligned) * sizeof(FloatAB);
    }

    // block_id to matrix tile idx (m0, n0) mapping are controlled by {M01, N01}
    __host__ __device__ static constexpr bool
    CheckValidity(const AGridDesc_K0_M_K1& a_grid_desc_k0_m_k1,
                  const BGridDesc_K0_N_K1& b_grid_desc_k0_n_k1,
                  const CGridDesc_M_N& c_grid_desc_m_n,
                  index_t M01,
                  index_t N01)
    {
        static_assert(is_known_at_compile_time<remove_cv_t<decltype(K1)>>::value,
                      "wrong! K1 need to be known at compile-time");

        static_assert((MPerBlock % (MPerXDL * MXdlPerWave) == 0) &&
                          (NPerBlock % (NXdlPerWave * NPerXDL)) == 0,
                      "Invalid tuning param!");

        const auto M  = a_grid_desc_k0_m_k1.GetLength(I1);
        const auto N  = b_grid_desc_k0_n_k1.GetLength(I1);
        const auto K0 = a_grid_desc_k0_m_k1.GetLength(I0);

        if(!(M == c_grid_desc_m_n.GetLength(I0) && N == c_grid_desc_m_n.GetLength(I1) &&
             K0 == b_grid_desc_k0_n_k1.GetLength(I0) && K1 == a_grid_desc_k0_m_k1.GetLength(I2) &&
             K1 == b_grid_desc_k0_n_k1.GetLength(I2)))
            return false;

        if(!(M % MPerBlock == 0 && N % NPerBlock == 0 && K0 % K0PerBlock == 0))
            return false;

        // 2-stage prefetch currently only support even number of K0 loop
        // TODO: add support for odd number of K0 loop
        if(!((K0 / K0PerBlock) % MultiK0 == 0))
        {
            return false;
        }

        // check M01, N01
        constexpr auto M1 = Number<MPerBlock>{};
        constexpr auto N1 = Number<NPerBlock>{};

        const auto M0 = M / M1;
        const auto N0 = N / N1;

        if(!(M0 % M01 == 0 && N0 % N01 == 0))
            return false;

        // TODO: also check validity of all components (blockwise-copy, threadwise-copy, etc)
        return true;
    }

    __host__ __device__ static constexpr index_t
    CalculateGridSize(const CGridDesc_M_N& c_grid_desc_m_n)
    {
        const auto M = c_grid_desc_m_n.GetLength(I0);
        const auto N = c_grid_desc_m_n.GetLength(I1);

        const index_t grid_size = (M / M0PerBlock) * (N / N1PerBlock);

        return grid_size;
    }

    // TODO move this function into GEMM-pipeline class
    __host__ __device__ static constexpr bool CalculateHasMainK0BlockLoop(index_t K0)
    {
        const bool has_main_k0_block_loop = (K0 / (MultiK0 * K0PerBlock)) > 1;

        return has_main_k0_block_loop;
    }

    __host__ __device__ static constexpr auto
    MakeB0GridDescriptor_K0_K1_K2_N0_N1_N2_N3_K3(const B0GridDesc_K0_N_K1& b0_grid_desc_k0_n_k1)
    {
        const auto K0 = b0_grid_desc_k0_n_k1.GetLength(I0);
        const auto N  = b0_grid_desc_k0_n_k1.GetLength(I1);

        const auto b0_griddesc_k0_nblockid_nrepeat_waves_nperxdlops_k1 = transform_tensor_descriptor(
            b0_grid_desc_k0_n_k1,
            make_tuple(make_unmerge_transform(
                           make_tuple(K0 / K0PerBlock, xdlops_gemm0.K0PerXdlops, K0PerThread)),
                       make_unmerge_transform(make_tuple(
                           N / (NXdlPerWave * NWaves * NPerXDL), NXdlPerWave, NWaves, NPerXDL)),
                       make_pass_through_transform(K1)),
            make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{}),
            make_tuple(Sequence<0, 1, 2>{}, Sequence<3, 4, 5, 6>{}, Sequence<7>{}));
        return b0_griddesc_k0_nblockid_nrepeat_waves_nperxdlops_k1;
    }

    __device__ static auto GetWaveIdx()
    {
        const index_t thread_id = get_thread_local_1d_id();

        constexpr auto threadid_to_wave_idx_adaptor = make_single_stage_tensor_adaptor(
            make_tuple(make_merge_transform(make_tuple(MWaves, NWaves, WaveSize))),
            make_tuple(Sequence<0, 1, 2>{}),
            make_tuple(Sequence<0>{}));

        return threadid_to_wave_idx_adaptor.CalculateBottomIndex(make_multi_index(thread_id));
    }

    __device__ static auto GetWaveKNIdx(const index_t thread_id)
    {
        constexpr auto wave_threadid_to_nk_idx_adaptor = make_single_stage_tensor_adaptor(
            make_tuple(make_merge_transform(make_tuple(xdlops_gemm.K0PerXdlops, NPerXDL))),
            make_tuple(Sequence<0, 1>{}),
            make_tuple(Sequence<0>{}));

        return wave_threadid_to_nk_idx_adaptor.CalculateBottomIndex(make_multi_index(thread_id));
    }

    __host__ __device__ static constexpr auto
    MakeCGridDescriptor_M0_N0_M1_N1_M2_M3_M4_N2(const CGridDesc_M_N& c_grid_desc_m_n)
    {
        constexpr auto max_lds_align = K1;

        // A matrix in LDS memory, dst of blockwise copy
        constexpr auto a_block_desc_k0_m_k1 = [&]() {
            if constexpr(ABlockLdsExtraM)
            {
                return make_naive_tensor_descriptor(
                    make_tuple(Number<K0PerBlock>{}, Number<MPerBlock>{}, K1),
                    make_tuple(Number<MPerBlock + 1>{} * K1, K1, I1));
            }
            else
            {
                return make_naive_tensor_descriptor_aligned(
                    make_tuple(Number<K0PerBlock>{}, Number<MPerBlock>{}, K1), max_lds_align);
            }
        }();

        // B matrix threadwise copy
        constexpr auto b_thread_desc_k0_k1_k2_n0_n1_n2_n3_k3 =
            make_naive_tensor_descriptor_packed(make_tuple(I1,
                                                           I1,
                                                           Number<K0PerThread>{}, // K0PerThread
                                                           I1,                    // NBlockId
                                                           Number<NXdlPerWave>{}, // repeat
                                                           I1,                    // waves
                                                           I1,                    // NPerXdlops
                                                           Number<K1>{}));

        using BlockwiseGemm = BlockwiseGemmXdlops_k0mk1_k0nk1_m0n0m1n1m2m3m4n2_v1r1<
            BlockSize,
            FloatAB,
            FloatAcc,
            decltype(a_block_desc_k0_m_k1),
            decltype(b_thread_desc_k0_k1_k2_n0_n1_n2_n3_k3),
            MPerBlock,
            NPerBlock,
            K0PerBlock,
            MPerXDL,
            NPerXDL,
            MXdlPerWave,
            NXdlPerWave,
            K1>;

        return BlockwiseGemm::MakeCGridDescriptor_M0_N0_M1_N1_M2_M3_M4_N2(c_grid_desc_m_n);
    }

    // return block_id to C matrix tile idx (m0, n0) mapping
    __host__ __device__ static constexpr auto
    MakeDefaultBlock2CTileMap(const CGridDesc_M_N& c_grid_desc_m_n, index_t M01, index_t N01)
    {
        const auto M = c_grid_desc_m_n.GetLength(I0);
        const auto N = c_grid_desc_m_n.GetLength(I1);

        constexpr auto M1 = Number<MPerBlock>{};
        constexpr auto N1 = Number<NPerBlock>{};

        const auto M0 = M / M1;
        const auto N0 = N / N1;

        const auto M00 = M0 / M01;
        const auto N00 = N0 / N01;

        const auto m00_m01_n00_n01_to_m0_n0_block_cluster_adaptor =
            make_single_stage_tensor_adaptor(
                make_tuple(make_unmerge_transform(make_tuple(M00, M01)),
                           make_unmerge_transform(make_tuple(N00, N01))),
                make_tuple(Sequence<0>{}, Sequence<1>{}),
                make_tuple(Sequence<0, 2>{}, Sequence<1, 3>{}));

        const auto cblockid_to_m00_m01_n00_n01_block_cluster_adaptor =
            make_single_stage_tensor_adaptor(
                make_tuple(make_merge_transform(make_tuple(M00, N00, M01, N01))),
                make_tuple(Sequence<0, 1, 2, 3>{}),
                make_tuple(Sequence<0>{}));

        const auto cblockid_to_m0_n0_block_cluster_adaptor =
            chain_tensor_adaptors(m00_m01_n00_n01_to_m0_n0_block_cluster_adaptor,
                                  cblockid_to_m00_m01_n00_n01_block_cluster_adaptor);

        return cblockid_to_m0_n0_block_cluster_adaptor;
    }

    using CGridDesc_M0_N0_M1_N1_M2_M3_M4_N2 =
        decltype(MakeCGridDescriptor_M0_N0_M1_N1_M2_M3_M4_N2(CGridDesc_M_N{}));
    using DefaultBlock2CTileMap = decltype(MakeDefaultBlock2CTileMap(CGridDesc_M_N{}, 1, 1));
    using B0GridDesc_K0_K1_K2_N0_N1_N2_N3_K3 =
        decltype(MakeB0GridDescriptor_K0_K1_K2_N0_N1_N2_N3_K3(B0GridDesc_K0_N_K1{}));

    using TypeConvertFp32ToFp16Functor =
        ck::tensor_operation::element_wise::UnaryTypeConvert<ck::half_t, float>;

    template <bool HasMainK0BlockLoop, typename Block2CTileMap = DefaultBlock2CTileMap>
    __device__ static void
    Run(const FloatAB* __restrict__ p_a_grid,
        const FloatAB* __restrict__ p_b0_grid,
        const FloatAB* __restrict__ p_b1_grid,
        FloatC* __restrict__ p_c_grid,
        void* __restrict__ p_shared,
        const AGridDesc_K0_M_K1& a_grid_desc_k0_m_k1,
        const B0GridDesc_K0_K1_K2_N0_N1_N2_N3_K3 b0_grid_desc_k0_k1_k2_n0_n1_n2_n3_k3,
        const B1GridDesc_K0_N_K1& b1_grid_desc_k0_n_k1, 
        const CGridDesc_M0_N0_M1_N1_M2_M3_M4_N2& c_grid_desc_m0_n0_m1_n1_m2_m3_m4_n2,
        const AElementwiseOperation& a_element_op,
        const BElementwiseOperation& b_element_op,
        const CElementwiseOperation& c_element_op,
        const Block2CTileMap& block_2_ctile_map)
    {
        const auto a_grid_buf = make_dynamic_buffer<AddressSpaceEnum::Global>(
            p_a_grid, a_grid_desc_k0_m_k1.GetElementSpaceSize());
        const auto b0_grid_buf = make_dynamic_buffer<AddressSpaceEnum::Global>(
            p_b0_grid, b0_grid_desc_k0_k1_k2_n0_n1_n2_n3_k3.GetElementSpaceSize());
        const auto b1_grid_buf = make_dynamic_buffer<AddressSpaceEnum::Global>(
            p_b1_grid, b1_grid_desc_k0_n_k1.GetElementSpaceSize());
        auto c_grid_buf = make_dynamic_buffer<AddressSpaceEnum::Global>(
            p_c_grid, c_grid_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetElementSpaceSize());

        const auto Gemm0K0 = a_grid_desc_k0_m_k1.GetLength(I0);

        // divide block work by [M, N]
        const auto block_work_idx =
            block_2_ctile_map.CalculateBottomIndex(make_multi_index(get_block_1d_id()));

        // HACK: this force m/n_block_data_idx_on_grid into SGPR
        const index_t m_block_data_idx_on_grid =
            __builtin_amdgcn_readfirstlane(block_work_idx[I0] * MPerBlock);

        const index_t n_block_data_idx_on_grid =
            __builtin_amdgcn_readfirstlane(block_work_idx[I1] * NPerBlock);

        // A matrix in LDS memory, dst of blockwise copy
        constexpr auto a_block_desc_k0_m_k1 = GetABlockDescriptor_K0PerBlock_MPerBlock_K1();

        // B1 matrix in LDS memory, dst of blockwise copy
        constexpr auto b1_block_desc_bk0_n_bk1 = GetB1BlockDescriptor_K0PerBlock_NPerBlock_K1();

        // A matrix blockwise copy
        auto a_blockwise_copy =
            ThreadGroupTensorSliceTransfer_v4r1<ThisThreadBlock,
                                                AElementwiseOperation,
                                                ck::tensor_operation::element_wise::PassThrough,
                                                InMemoryDataOperationEnum::Set,
                                                Sequence<K0PerBlock * MultiK0, MPerBlock, K1>,
                                                ABlockTransferThreadClusterLengths_K0_M_K1,
                                                ABlockTransferThreadClusterArrangeOrder,
                                                FloatAB,
                                                FloatAB,
                                                decltype(a_grid_desc_k0_m_k1),
                                                decltype(a_block_desc_k0_m_k1),
                                                ABlockTransferSrcAccessOrder,
                                                Sequence<1, 0, 2>,
                                                ABlockTransferSrcVectorDim,
                                                2,
                                                ABlockTransferSrcScalarPerVector,
                                                ABlockTransferDstScalarPerVector_K1,
                                                1,
                                                1,
                                                AThreadTransferSrcResetCoordinateAfterRun,
                                                true,
                                                1>(
                a_grid_desc_k0_m_k1,
                make_multi_index(0, m_block_data_idx_on_grid, 0),
                a_element_op,
                a_block_desc_k0_m_k1,
                make_multi_index(0, 0, 0),
                ck::tensor_operation::element_wise::PassThrough{});

        ignore = b_element_op;
        // B matrix threadwise copy
        constexpr auto b_thread_desc_k0_k1_k2_n0_n1_n2_n3_k3 =
            make_naive_tensor_descriptor_packed(make_tuple(I1,
                                                           I1,
                                                           Number<K0PerThread>{}, // K0PerThread
                                                           I1,                    // NBlockId
                                                           Number<NXdlPerWave>{}, // repeat
                                                           I1,                    // waves
                                                           I1,                    // NPerXdlops
                                                           Number<K1>{}));

        StaticBuffer<AddressSpaceEnum::Vgpr,
                     FloatAB,
                     b_thread_desc_k0_k1_k2_n0_n1_n2_n3_k3.GetElementSpaceSize(),
                     true>
            b_thread_buf[MultiK0];

        const auto wave_id     = GetWaveIdx();
        const auto wave_k_n_id = GetWaveKNIdx(wave_id[I2]);

#if 0
        const index_t block_id  = get_block_1d_id();
        const index_t thread_id = get_thread_local_1d_id();
        printf("block id: %d  m blockid: %d n block id: %d ,thread id: %d, wave id :{%d %d %d} "
               "kn id: {%d %d}\n",
               block_id,
               block_work_idx[I0],
               block_work_idx[I1],
               thread_id,
               wave_id[I0],
               wave_id[I1],
               wave_id[I2],
               wave_k_n_id[I0],
               wave_k_n_id[I1]);
        printf("mfma thread k per xdlops: %d K0PerThread: %d HasMainK0BlockLoop: %d K0: %d  \t", 
                xdlops_gemm.K0PerXdlops, K0PerThread, HasMainK0BlockLoop, b0_grid_desc_k0_k1_k2_n0_n1_n2_n3_k3.GetLength(I0));
#endif

        auto b_threadwise_copy =
            ThreadwiseTensorSliceTransfer_v2<FloatAB,
                                             FloatAB,
                                             decltype(b0_grid_desc_k0_k1_k2_n0_n1_n2_n3_k3),
                                             decltype(b_thread_desc_k0_k1_k2_n0_n1_n2_n3_k3),
                                             Sequence<I1,
                                                      I1,
                                                      Number<K0PerThread>{},
                                                      I1,
                                                      Number<NXdlPerWave>{},
                                                      I1,
                                                      I1,
                                                      Number<K1>{}>,
                                             Sequence<0, 1, 2, 3, 4, 5, 6, 7>,
                                             7,
                                             BBlockTransferSrcScalarPerVector,
                                             BThreadTransferSrcResetCoordinateAfterRun,
                                             true>(
                b0_grid_desc_k0_k1_k2_n0_n1_n2_n3_k3,
                make_multi_index(
                    0, wave_k_n_id[I0], 0, block_work_idx[I1], 0, wave_id[I1], wave_k_n_id[I1], 0));

        // GEMM0 definition
        //   c_mtx += b_mtx * a_mtx
        //   c_mtx[MPerBlock, NPerBlock] is distributed among threads, and saved in
        //   register
        // sanity check
        auto blockwise_gemm = BlockwiseGemmXdlops_k0mk1_k0nk1_m0n0m1n1m2m3m4n2_v1r1<
            BlockSize,
            FloatAB,
            FloatAcc,
            decltype(a_block_desc_k0_m_k1),
            decltype(b_thread_desc_k0_k1_k2_n0_n1_n2_n3_k3),
            MPerBlock,
            NPerBlock,
            K0PerBlock,
            MPerXDL,
            NPerXDL,
            MXdlPerWave,
            NXdlPerWave,
            K1>{};

        auto c_thread_buf = blockwise_gemm.GetCThreadBuffer();

        // LDS allocation for A
        auto a_block_buf = make_dynamic_buffer<AddressSpaceEnum::Lds>(
            static_cast<FloatAB*>(p_shared), a_block_desc_k0_m_k1.GetElementSpaceSize());

        constexpr auto a_block_slice_copy_step  = make_multi_index(K0PerBlock * MultiK0, 0, 0);
        constexpr auto b_thread_slice_copy_step = make_multi_index(1, 0, 0, 0, 0, 0, 0, 0);

        // gridwise GEMM 0 pipeline
        static_assert(std::is_default_constructible_v<GridwiseGemmPipe0>);
        const auto gridwise_gemm_pipeline0 = GridwiseGemmPipe0{};

        const index_t num_k_block_main_loop = __builtin_amdgcn_readfirstlane(
            (a_grid_desc_k0_m_k1.GetLength(I0) * a_grid_desc_k0_m_k1.GetLength(I2)) /
            KPerBlock);

        gridwise_gemm_pipeline0.template Run<HasMainKBlockLoop,
                                            MultiK0>
            (a_grid_desc_k0_m_k1,
             a_block_desc_k0_m_k1,
             a_blockwise_copy,
             a_grid_buf,
             a_block_buf,
             a_block_slice_copy_step,
             b0_grid_desc_k0_k1_k2_n0_n1_n2_n3_k3,
             b_thread_desc_k0_k1_k2_n0_n1_n2_n3_k3,
             b_threadwise_copy,
             b_grid_buf,
             b_thread_buf,
             b_thread_slice_copy_step,
             blockwise_gemm,
             c_thread_buf,
             num_k_block_main_loop);

        // gemm 1 O=PV
        // Gemm1
        //
        // Acc matrix threadwise copy: AccVGPR to VGPR and downcast to A data type
        constexpr auto acc_block_slice_copy_step = make_multi_index(Gemm1KPerBlock / AccK1, 0, 0);
        constexpr auto b1_block_slice_copy_step = make_multi_index(Gemm1KPerBlock / B1K1, 0, 0);

        constexpr auto acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2 =
            blockwise_gemm.GetCThreadDescriptor_M0_N0_M1_N1_M2_M3_M4_N2();
        //constexpr auto acc_thread_desc_m0_n0_m1_n1_m2_n2_n3_n4 =
        //    blockwise_gemm.GetCThreadDescriptor_M0_N0_M1_N1_M2_N2_N3_N4();
        constexpr auto m0 = acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I0);
        constexpr auto n0 = acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I1);
        constexpr auto m1 = acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I2);
        constexpr auto n1 = acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I3);
        constexpr auto m2 = acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I4);
        constexpr auto m3 = acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I5);
        constexpr auto m4 = acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I6);
        constexpr auto n2 = acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I7);

        // acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2 to a1_thread_desc_k0_m_k1
        // m0_m1_m2_m3 -> k0
        // n0_n1_n2 -> m
        // m4 -> k1
        // typical case: m0 = MRepeat, n0 = NRepeat, m4 = 4, the others are all 1
        constexpr auto a1_thread_desc_k0_m_k1 = transform_tensor_descriptor(
            acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2,
            make_tuple(make_merge_transform(make_tuple(m0, m1, m2, m3)),
                       make_merge_transform(make_tuple(n0, n1, n2)),
                       make_pass_through_transform(m4)),
            make_tuple(Sequence<0, 2, 4, 5>{}, Sequence<1, 3, 7>{}, Sequence<6>{}),
            make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{}));

        
        // B1 matrix blockwise copy
        auto b1_blockwise_copy =
            ThreadGroupTensorSliceTransfer_v4r1<ThisThreadBlock,
                                                BElementwiseOperation,
                                                tensor_operation::element_wise::PassThrough,
                                                InMemoryDataOperationEnum::Set,
                                                Sequence<B1K0, Gemm1NPerBlock, B1K1>,
                                                B1BlockTransferThreadClusterLengths_BK0_N_BK1,
                                                B1BlockTransferThreadClusterArrangeOrder,
                                                FloatAB,
                                                FloatAB,
                                                decltype(b1_grid_desc_bk0_n_bk1),
                                                decltype(b1_block_desc_bk0_n_bk1),
                                                B1BlockTransferSrcAccessOrder,
                                                Sequence<1, 0, 2>,
                                                B1BlockTransferSrcVectorDim,
                                                2,
                                                B1BlockTransferSrcScalarPerVector,
                                                B1BlockTransferDstScalarPerVector_BK1,
                                                1,
                                                1,
                                                B1ThreadTransferSrcResetCoordinateAfterRun,
                                                true,
                                                NumGemmKPrefetchStage>(
                b1_grid_desc_bk0_n_bk1,
                make_multi_index(0, gemm1_n_block_data_idx_on_grid, 0),
                b_element_op,
                b1_block_desc_bk0_n_bk1,
                make_multi_index(0, 0, 0),
                tensor_operation::element_wise::PassThrough{});

        auto a1_thread_buf = make_static_buffer<AddressSpaceEnum::Vgpr, FloatAB>(
            acc_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetElementSpaceSize());

        // reuse LDS space for gemm0's a_block_buf
        auto b1_block_buf = make_dynamic_buffer<AddressSpaceEnum::Lds>(
            static_cast<FloatAB*>(p_shared),
            b1_block_desc_bk0_n_bk1.GetElementSpaceSize());


        constexpr auto a1_thread_slice_copy_step  = make_multi_index(Gemm1K0PerBlock, 0, 0, 0, 0, 0, 0, 0);
        constexpr auto b1_block_slice_copy_step = make_multi_index(Gemm1K0PerBlock, 0, 0);

        // GEMM1 definition
        //   c_mtx += a_mtx * b_mtx
        //   c_mtx[MPerBlock, NPerBlock] is distributed among threads, and saved in
        //   register
        // sanity check
        auto blockwise_gemm1 = BlockwiseGemmXdlops_k0mk1_k0nk1_m0n0m1n1m2m3m4n2_skip_a_lds<
            BlockSize,
            FloatAB,
            FloatAcc,
            decltype(b1_block_desc_bk0_n_bk1),
            MPerBlock,
            NPerBlock,
            Gemm1K0PerBlock,
            MPerXDL,
            NPerXDL,
            MXdlPerWave,
            NXdlPerWave,
            K1>{};

        auto c1_thread_buf = blockwise_gemm1.GetCThreadBuffer();

        // gridwise GEMM 0 pipeline
        static_assert(std::is_default_constructible_v<GridwiseGemmPipe1>);
        const auto gridwise_gemm_pipeline1 = GridwiseGemmPipe1{};

        const index_t num_k_block_main_loop_1 = __builtin_amdgcn_readfirstlane(
            (a_grid_desc_k0_m_k1.GetLength(I0) * a_grid_desc_k0_m_k1.GetLength(I2)) /
            KPerBlock);

        gridwise_gemm_pipeline1.template Run<TypeConvertFp32ToFp16Functor,
                                             MultiK0>
            (a1_thread_desc_k0_m_k1,
             c_thread_buf,
             a1_thread_buf,
             a1_thread_slice_copy_step,
             b1_grid_desc_bk0_n_bk1,
             b1_block_desc_bk0_n_bk1,
             b1_blockwise_copy,
             b1_grid_buf,
             b1_block_buf,
             b1_block_slice_copy_step,
             blockwise_gemm,
             c1_thread_buf,
             num_k_block_main_loop);
        

        // output: register to global memory
        {
            constexpr auto c_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2 =
                blockwise_gemm.GetCThreadDescriptor_M0_N0_M1_N1_M2_M3_M4_N2();

            constexpr auto c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2 =
                blockwise_gemm.GetCBlockDescriptor_M0_N0_M1_N1_M2_M3_M4_N2();

            constexpr auto M0 = c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I0);
            constexpr auto N0 = c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I1);
            constexpr auto M1 = c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I2);
            constexpr auto N1 = c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I3);
            constexpr auto M2 = c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I4);
            constexpr auto M3 = c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I5);
            constexpr auto M4 = c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I6);
            constexpr auto N2 = c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2.GetLength(I7);

            // calculate origin of thread output tensor on global memory
            //     blockwise GEMM c matrix starting index
            const auto c_thread_mtx_on_block =
                blockwise_gemm.CalculateCThreadOriginDataIndex(I0, I0, I0, I0);

            const index_t m_thread_data_on_grid =
                m_block_data_idx_on_grid + c_thread_mtx_on_block[I0];

            const index_t n_thread_data_on_grid =
                n_block_data_idx_on_grid + c_thread_mtx_on_block[I1];

            const auto m_thread_data_on_grid_to_m0_m1_m2_m3_m4_adaptor =
                make_single_stage_tensor_adaptor(
                    make_tuple(make_merge_transform(make_tuple(M0, M1, M2, M3, M4))),
                    make_tuple(Sequence<0, 1, 2, 3, 4>{}),
                    make_tuple(Sequence<0>{}));

            const auto m_thread_data_on_grid_idx =
                m_thread_data_on_grid_to_m0_m1_m2_m3_m4_adaptor.CalculateBottomIndex(
                    make_multi_index(m_thread_data_on_grid));

            const auto n_thread_data_on_grid_to_n0_n1_n2_adaptor = make_single_stage_tensor_adaptor(
                make_tuple(make_merge_transform(make_tuple(N0, N1, N2))),
                make_tuple(Sequence<0, 1, 2>{}),
                make_tuple(Sequence<0>{}));

            const auto n_thread_data_on_grid_idx =
                n_thread_data_on_grid_to_n0_n1_n2_adaptor.CalculateBottomIndex(
                    make_multi_index(n_thread_data_on_grid));

            auto c_thread_copy =
                ThreadwiseTensorSliceTransfer_v1r3<FloatAcc,
                                                   FloatC,
                                                   decltype(c_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2),
                                                   decltype(c_grid_desc_m0_n0_m1_n1_m2_m3_m4_n2),
                                                   CElementwiseOperation,
                                                   Sequence<M0, N0, I1, I1, M2, I1, M4, I1>,
                                                   CThreadTransferSrcDstAccessOrder,
                                                   CThreadTransferSrcDstVectorDim,
                                                   CThreadTransferDstScalarPerVector,
                                                   CGlobalMemoryDataOperation,
                                                   1,
                                                   true>{
                    c_grid_desc_m0_n0_m1_n1_m2_m3_m4_n2,
                    make_multi_index(m_thread_data_on_grid_idx[I0],
                                     n_thread_data_on_grid_idx[I0],
                                     m_thread_data_on_grid_idx[I1],
                                     n_thread_data_on_grid_idx[I1],
                                     m_thread_data_on_grid_idx[I2],
                                     m_thread_data_on_grid_idx[I3],
                                     m_thread_data_on_grid_idx[I4],
                                     n_thread_data_on_grid_idx[I2]),
                    c_element_op};

            c_thread_copy.Run(c_thread_desc_m0_n0_m1_n1_m2_m3_m4_n2,
                              make_tuple(I0, I0, I0, I0, I0, I0, I0, I0),
                              c_thread_buf,
                              c_grid_desc_m0_n0_m1_n1_m2_m3_m4_n2,
                              c_grid_buf);
        }
    }
};

} // namespace ck