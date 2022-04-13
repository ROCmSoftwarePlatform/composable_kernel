#include <stdlib.h>
#include "config.hpp"
#include "device_gemm_dlops.hpp"
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

static constexpr auto GemmDefault = ck::tensor_operation::device::GemmSpecialization::Default;

// Compilation parameters for a[k, m] * b[k, n] = c[m, n]
using device_gemm_dlops_f32_f32_f32_km_kn_mn_instances =
    std::tuple<
        // clang-format off
        //  ##########| AData| BData| CData| AccData| ALayout| BLayout| CLayout|           A|           B|           C|          GEMM| Block|  MPer|  NPer| K0Per| K1|      M1Per|      N1Per|   KPer|  M11N11Thread|  M11N11Thread|     ABlockTransfer|       ABlockTransfer| ABlockTransfer| ABlockTransfer|
        //  ##########|  Type|  Type|  Type|    Type|        |        |        | Elementwise| Elementwise| Elementwise|Spacialization|  Size| Block| Block| Block|   | ThreadM111| ThreadN111| Thread| ClusterM110Xs| ClusterN110Xs| ThreadSliceLengths| ThreadClusterLengths|  ThreadCluster|      SrcAccess|
        //  ##########|      |      |      |        |        |        |        |   Operation|   Operation|   Operation|              |      |      |      |      |   |           |           |       |              |              |        K0_M0_M1_K1|          K0_M0_M1_K1|   ArrangeOrder|          Order|
        //  ##########|      |      |      |        |        |        |        |            |            |            |              |      |      |      |      |   |           |           |       |              |              |                   |                     |               |               |               |               |               |          |                |               |               |              |               |               |          |                |                |
        DeviceGemmDlops<  F32,   F32,   F32,     F32,     Col,     Row,     Row, PassThrough, PassThrough, PassThrough,   GemmDefault,   256,   128,   128,     8,  2,          4,          4,      1,       S<8, 2>,       S<8, 2>,      S<4, 1, 1, 2>,      S<2, 1, 128, 1>,  S<1, 2, 0, 3>,  S<1, 2, 0 ,3>, S<4, 1, 1, 2>, S<1, 2, 0, 3>, S<1, 1, 1, 2>, S<4, 1, 1, 2>, S<2, 1, 128, 1>, S<1, 2, 0, 3>, S<1, 2, 0, 3>, S<4, 1, 1, 2>, S<1, 2, 0 3>, S<1, 1, 1, 2>, S<0, 1, 2, 3, 4, 5>, 5, 4> 
        // clang-format on
        >;

void add_device_gemm_dlops_f32_f32_f32_km_kn_mn_instances(
    std::vector<DeviceGemmPtr<PassThrough, PassThrough, PassThrough>>& instances)
{
    add_device_operation_instances(instances, device_gemm_dlops_f32_f32_f32_km_kn_mn_instances{});
}

} // namespace device_gemm_instance
} // namespace device
} // namespace tensor_operation
} // namespace ck