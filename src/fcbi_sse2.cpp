#include <cstring>

#include "fcbi.h"
#include "VCL2/vectorclass.h"

template <typename T>
void phase1_sse2(const uint8_t* srcp_, uint8_t* __restrict dstp_, int width, const int height, int spitch, int dpitch) noexcept
{
    spitch /= sizeof(T);
    dpitch /= sizeof(T);
    const T* srcp{ reinterpret_cast<const T*>(srcp_) };
    T* __restrict dstp{ reinterpret_cast<T*>(dstp_) };

    const auto zero{ zero_si128() };

    dstp[-2] = dstp[-1] = srcp[0];

    if constexpr (std::is_same_v<T, uint8_t>)
    {
        for (int x = 0; x < width - 1; x += 16)
        {
            const auto s0{ Vec16uc().load(srcp + x) };
            auto s1{ Vec16uc().load(srcp + x + 1) };
            s1 = avg(s0, s1);
            const auto d0{ blend16<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(s0, s1) };
            const auto d1{ blend16<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(s0, s1) };
            d0.store(dstp + 2 * x);
            d1.store(dstp + 2 * x + 16);
        }
    }
    else
    {
        for (int x = 0; x < width - 1; x += 8)
        {
            const auto s0{ Vec8us().load(srcp + x) };
            auto s1{ Vec8us().load(srcp + x + 1) };
            s1 = avg(s0, s1);
            const auto d0{ blend8<0, 8, 1, 9, 2, 10, 3, 11>(s0, s1) };
            const auto d1{ blend8<4, 12, 5, 13, 6, 14, 7, 15>(s0, s1) };
            d0.store(dstp + 2 * x);
            d1.store(dstp + 2 * x + 8);
        }
    }

    dstp[2 * width - 2] = dstp[2 * width - 1] = dstp[2 * width] = srcp[width - 1];
    srcp += spitch;
    dstp += 2 * dpitch;

    for (int y{ 1 }; y < height - 1; ++y)
    {
        dstp[-2] = dstp[-1] = srcp[0];

        if constexpr (std::is_same_v<T, uint8_t>)
        {
            for (int x = 0; x < width - 1; x += 16)
            {
                const auto s0{ Vec16uc().load(srcp + x) };
                const auto d0{ blend16<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(s0, zero) };
                const auto d1{ blend16<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(s0, zero) };
                d0.store(dstp + 2 * x);
                d1.store(dstp + 2 * x + 16);
            }
        }
        else
        {
            for (int x = 0; x < width - 1; x += 8)
            {
                const auto s0{ Vec8us().load(srcp + x) };
                const auto d0{ blend8<0, 8, 1, 9, 2, 10, 3, 11>(s0, zero) };
                const auto d1{ blend8<4, 12, 5, 13, 6, 14, 7, 15>(s0, zero) };
                d0.store(dstp + 2 * x);
                d1.store(dstp + 2 * x + 8);
            }
        }

        dstp[2 * width - 2] = dstp[2 * width - 1] = dstp[2 * width] = srcp[width - 1];
        srcp += spitch;
        dstp += 2 * dpitch;
    }

    dstp[-2] = dstp[-1] = srcp[0];

    if constexpr (std::is_same_v<T, uint8_t>)
    {
        for (int x = 0; x < width - 1; x += 16)
        {
            const auto s0{ Vec16uc().load(srcp + x) };
            auto s1{ Vec16uc().load(srcp + x + 1) };
            s1 = avg(s0, s1);
            const auto d0{ blend16<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(s0, s1) };
            const auto d1{ blend16<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(s0, s1) };
            d0.store(dstp + 2 * x);
            d1.store(dstp + 2 * x + 16);
        }
    }
    else
    {
        for (int x = 0; x < width - 1; x += 8)
        {
            const auto s0{ Vec8us().load(srcp + x) };
            auto s1{ Vec8us().load(srcp + x + 1) };
            s1 = avg(s0, s1);
            const auto d0{ blend8<0, 8, 1, 9, 2, 10, 3, 11>(s0, s1) };
            const auto d1{ blend8<4, 12, 5, 13, 6, 14, 7, 15>(s0, s1) };
            d0.store(dstp + 2 * x);
            d1.store(dstp + 2 * x + 8);
        }
    }

    dstp[2 * width - 2] = dstp[2 * width - 1] = dstp[2 * width] = srcp[width - 1];
    dstp -= 2;
    memcpy(dstp + dpitch, dstp, (2 * width + 4) * sizeof(T));
    dstp += dpitch;
    memcpy(dstp + dpitch, dstp, (2 * width + 4) * sizeof(T));
}

template void phase1_sse2<uint8_t>(const uint8_t* srcp_, uint8_t* __restrict dstp_, int width, const int height, int spitch, int dpitch) noexcept;
template void phase1_sse2<uint16_t>(const uint8_t* srcp_, uint8_t* __restrict dstp_, int width, const int height, int spitch, int dpitch) noexcept;
