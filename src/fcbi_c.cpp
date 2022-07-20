#include <algorithm>
#include <cstring>

#include "fcbi.h"

#if defined(__clang__)
// Check clang first. clang-cl also defines __MSC_VER
// We set MSVC because they are mostly compatible
#   define CLANG
#if defined(_MSC_VER)
#   define MSVC
#   define AVS_FORCEINLINE __attribute__((always_inline))
#else
#   define AVS_FORCEINLINE __attribute__((always_inline)) inline
#endif
#elif   defined(_MSC_VER)
#   define MSVC
#   define MSVC_PURE
#   define AVS_FORCEINLINE __forceinline
#elif defined(__GNUC__)
#   define GCC
#   define AVS_FORCEINLINE __attribute__((always_inline)) inline
#else
#   error Unsupported compiler.
#   define AVS_FORCEINLINE inline
#   undef __forceinline
#   define __forceinline inline
#endif

template <typename T>
static AVS_FORCEINLINE int mean(T x, T y) noexcept
{
    return (x + y + 1) >> 1;
}

template <typename T>
void phase1_c(const uint8_t* srcp_, uint8_t* __restrict dstp_, int width, const int height, int spitch, int dpitch) noexcept
{
    spitch /= sizeof(T);
    dpitch /= sizeof(T);
    const T* srcp{ reinterpret_cast<const T*>(srcp_) };
    T* __restrict dstp{ reinterpret_cast<T*>(dstp_) };

    dstp[-2] = dstp[-1] = srcp[0];

    for (int x{ 0 }; x < width - 1; ++x)
    {
        dstp[2 * x] = srcp[x];
        dstp[2 * x + 1] = mean<T>(srcp[x], srcp[x + 1]);
    }

    dstp[2 * width - 2] = dstp[2 * width - 1] = dstp[2 * width] = srcp[width - 1];
    srcp += spitch;
    dstp += 2 * dpitch;

    for (int y{ 1 }; y < height - 1; ++y)
    {
        dstp[-2] = dstp[-1] = srcp[0];

        if constexpr (std::is_same_v<T, uint8_t>)
        {
            uint16_t* d16{ reinterpret_cast<uint16_t*>(dstp) };
            for (int x = 0; x < width - 1; ++x)
                d16[x] = static_cast<uint16_t>(srcp[x]);
        }
        else
        {
            uint32_t* d32{ reinterpret_cast<uint32_t*>(dstp) };
            for (int x = 0; x < width - 1; ++x)
                d32[x] = static_cast<uint32_t>(srcp[x]);
        }

        dstp[2 * width - 2] = dstp[2 * width - 1] = dstp[2 * width] = srcp[width - 1];
        srcp += spitch;
        dstp += 2 * dpitch;
    }

    dstp[-2] = dstp[-1] = srcp[0];

    for (int x = 0; x < width - 1; ++x)
    {
        dstp[2 * x] = srcp[x];
        dstp[2 * x + 1] = mean<T>(srcp[x], srcp[x + 1]);
    }

    dstp[2 * width - 2] = dstp[2 * width - 1] = dstp[2 * width] = srcp[width - 1];
    dstp -= 2;
    memcpy(dstp + dpitch, dstp, (2 * width + 4) * sizeof(T));
    dstp += dpitch;
    memcpy(dstp + dpitch, dstp, (2 * width + 4) * sizeof(T));
}

template void phase1_c<uint8_t>(const uint8_t* srcp_, uint8_t* __restrict dstp_, int width, const int height, int spitch, int dpitch) noexcept;
template void phase1_c<uint16_t>(const uint8_t* srcp_, uint8_t* __restrict dstp_, int width, const int height, int spitch, int dpitch) noexcept;

static AVS_FORCEINLINE int abs_diff(int x, int y)
{
    return x > y ? x - y : y - x;
}

static AVS_FORCEINLINE bool is_edge(int v1, int v2, int p1, int p2, int tm)
{
    if (abs_diff(v1, v2) < tm) {
        return false;
    }
    return !(v1 < tm&& v2 < tm&& abs_diff(p1, p2) < tm * 2);
}

template <typename T, bool EDGE>
void phase2_c(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept
{
    pitch /= sizeof(T);

    const int p2{ 2 * pitch };
    const T* s0{ reinterpret_cast<const T*>(ptr) };
    const T* s1{ s0 };
    const T* s2{ s1 + p2 };
    const T* s3{ s2 + p2 };

    T* __restrict dstp{ reinterpret_cast<T*>(ptr) + pitch };

    for (int y{ 1 }; y < height - 1; y += 2)
    {
        dstp[0] = mean<T>(s1[0], s2[0]);

        for (int x{ 1 }; x < width - 2; x += 2)
        {
            const int p1{ s1[x - 1] + s2[x + 1] };
            const int p2{ s1[x + 1] + s2[x - 1] };

            if constexpr (EDGE)
            {
                const int v1{ abs_diff(s1[x - 1], s2[x + 1]) };
                const int v2{ abs_diff(s1[x + 1], s2[x - 1]) };

                if (is_edge(v1, v2, p1, p2, tm))
                {
                    if (v1 < v2)
                        dstp[x] = (p1 + 1) >> 1;
                    else
                        dstp[x] = (p2 + 1) >> 1;
                    continue;
                }
            }

            const int h1{ s0[x + 1] + s1[x + 3] + s2[x - 3] + s3[x - 1] + p1 - 3 * p2 };
            const int h2{ s0[x - 1] + s1[x - 3] + s2[x + 3] + s3[x + 1] + p2 - 3 * p1 };

            if (std::abs(h1) < std::abs(h2))
                dstp[x] = (p1 + 1) / 2;
            else
                dstp[x] = (p2 + 1) / 2;
        }

        dstp[width - 2] = dstp[width - 1] = mean<T>(s1[width - 2], s2[width - 2]);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 += p2;
        dstp += p2;
    }
}

template void phase2_c<uint8_t, true>(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept;
template void phase2_c<uint8_t, false>(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept;

template void phase2_c<uint16_t, true>(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept;
template void phase2_c<uint16_t, false>(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept;

template <typename T, bool EDGE>
void phase3_c(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept
{
    pitch /= sizeof(T);

    const T* s0{ reinterpret_cast<const T*>(ptr) };
    const T* s1{ reinterpret_cast<const T*>(ptr) };
    const T* s2{ s1 + pitch };
    const T* s3{ s2 + pitch };
    const T* s4{ s3 + pitch };

    T* __restrict dstp{ reinterpret_cast<T*>(ptr) + pitch };

    for (int y{ 1 }; y < height - 2; ++y)
    {
        for (int x{ 1 + (y & 1) }; x < width - 2; x += 2)
        {
            const int p1{ s2[x - 1] + s2[x + 1] };
            const int p2{ s1[x] + s3[x] };

            if constexpr (EDGE)
            {
                const int v1{ abs_diff(s2[x - 1], s2[x + 1]) };
                const int v2{ abs_diff(s1[x], s3[x]) };

                if (is_edge(v1, v2, p1, p2, tm))
                {
                    if (v1 < v2)
                        dstp[x] = (p1 + 1) / 2;
                    else
                        dstp[x] = (p2 + 1) / 2;
                    continue;
                }
            }

            const int h1{ s0[x - 1] + s0[x + 1] + s4[x - 1] + s4[x + 1] + p1 - 3 * p2 };
            const int h2{ s1[x - 2] + s1[x + 2] + s3[x - 2] + s3[x + 2] + p2 - 3 * p1 };
            if (std::abs(h1) < std::abs(h2))
                dstp[x] = (p1 + 1) / 2;
            else
                dstp[x] = (p2 + 1) / 2;
        }

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 += pitch;
        dstp += pitch;
    }
}

template void phase3_c<uint8_t, true>(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept;
template void phase3_c<uint8_t, false>(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept;

template void phase3_c<uint16_t, true>(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept;
template void phase3_c<uint16_t, false>(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept;
