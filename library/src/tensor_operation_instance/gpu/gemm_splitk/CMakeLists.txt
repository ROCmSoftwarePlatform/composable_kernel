set(DEVICE_GEMM_SPLITK_INSTANCE_SOURCE
   device_gemm_xdl_splitk_f32_f32_f32_mk_kn_mn_instance.cpp;
   device_gemm_xdl_splitk_f32_f32_f32_mk_nk_mn_instance.cpp;
   device_gemm_xdl_splitk_f32_f32_f32_km_kn_mn_instance.cpp;
   device_gemm_xdl_splitk_f32_f32_f32_km_nk_mn_instance.cpp;
   device_gemm_xdl_splitk_f16_f16_f16_mk_kn_mn_instance.cpp;
   device_gemm_xdl_splitk_f16_f16_f16_mk_nk_mn_instance.cpp;
   device_gemm_xdl_splitk_f16_f16_f16_km_kn_mn_instance.cpp;
   device_gemm_xdl_splitk_f16_f16_f16_km_nk_mn_instance.cpp;
)

add_library(device_gemm_splitk_instance OBJECT ${DEVICE_GEMM_SPLITK_INSTANCE_SOURCE})

target_compile_features(device_gemm_splitk_instance PUBLIC)
set_target_properties(device_gemm_splitk_instance PROPERTIES POSITION_INDEPENDENT_CODE ON)
