#ifndef CK_MATH_V2_HPP
#define CK_MATH_V2_HPP

#include <cmath>
#include "data_type.hpp"
#include "type.hpp"

namespace ck {
namespace math {

// math functions for the host,  some are implemented by calling C++ std functions

static inline __host__ float ck_abs(float x) { return std::abs(x); };

static inline __host__ double ck_abs(double x) { return std::abs(x); };

static inline __host__ int8_t ck_abs(int8_t x)
{
    int8_t sgn = x >> (8 - 1);

    return (x ^ sgn) - sgn;
};

static inline __host__ int32_t ck_abs(int32_t x)
{
    int32_t sgn = x >> (32 - 1);

    return (x ^ sgn) - sgn;
};

static inline __host__ half_t ck_abs(half_t x)
{
    uint16_t xx = ck::bit_cast<uint16_t>(x);

    uint16_t abs_xx = xx & 0x7fff;

    half_t abs_x = ck::bit_cast<half_t>(abs_xx);

    return abs_x;
};

static inline __host__ bool ck_isnan(float x) { return std::isnan(x); };

static inline __host__ bool ck_isnan(double x) { return std::isnan(x); };

static inline __host__ bool ck_isnan(int8_t x)
{
    (void)x;
    return false;
};

static inline __host__ bool ck_isnan(int32_t x)
{
    (void)x;
    return false;
};

static inline __host__ bool ck_isnan(half_t x)
{
    uint16_t xx = ck::bit_cast<uint16_t>(x);

    return (xx & 0x7FFF) > 0x7C00;
};

static inline __host__ float ck_sqrt(float x) { return std::sqrt(x); };

static inline __host__ double ck_sqrt(double x) { return std::sqrt(x); };

// math functions for the HIP kernel,  some are implemented by calling hip builtin functions

static inline __device__ float ck_abs(float x) { return abs(x); };

static inline __device__ double ck_abs(double x) { return abs(x); };

static inline __device__ int8_t ck_abs(int8_t x)
{
    int8_t sgn = x >> (8 - 1);

    return (x ^ sgn) - sgn;
};

static inline __device__ int32_t ck_abs(int32_t x)
{
    int32_t sgn = x >> (32 - 1);

    return (x ^ sgn) - sgn;
};

static inline __device__ half_t ck_abs(half_t x) { return __habs(x); };

static inline __device__ bool ck_isnan(float x) { return isnan(x); };

static inline __device__ bool ck_isnan(double x) { return isnan(x); };

static inline __device__ bool ck_isnan(int8_t x)
{
    (void)x;
    return false;
};

static inline __device__ bool ck_isnan(int32_t x)
{
    (void)x;
    return false;
};

static inline __device__ bool ck_isnan(half_t x) { return __hisnan(x); };

static inline __device__ float ck_sqrt(float x) { return sqrtf(x); };

static inline __device__ double ck_sqrt(double x) { return sqrt(x); };

} // namespace math
} // namespace ck

#endif
