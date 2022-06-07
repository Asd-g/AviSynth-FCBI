#include <algorithm>

#include "fcbi.h"
#include "VCL2/instrset.h"


template <typename T>
static AVS_FORCEINLINE int mean(T x, T y) noexcept
{
    return (x + y + 1) >> 1;
}

template <typename T>
static void phase1_c(const uint8_t* srcp_, uint8_t* __restrict dstp_, int width, const int height, int spitch, int dpitch) noexcept
{
    spitch /= sizeof(T);
    dpitch /= sizeof(T);
    width /= sizeof(T);
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
static void phase2_c(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept
{
    pitch /= sizeof(T);
    width /= sizeof(T);

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

template <typename T, bool EDGE>
static void phase3_c(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept
{
    pitch /= sizeof(T);
    width /= sizeof(T);

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

FCBI::FCBI(PClip _c, bool _e, int _t, int opt, IScriptEnvironment* env)
    : GenericVideoFilter(_c), tm(_t), v8(true)
{
    if (!vi.IsPlanar() || vi.IsRGB())
        env->ThrowError("FCBI: input clip is not planar YUV format");
    if (vi.ComponentSize() == 4)
        env->ThrowError("FCBI: bit depth of input clip is must be 8..16-bit");
    if (vi.NumComponents() == 4)
        env->ThrowError("FCBI: alpha is unsupported");
    if (vi.IsYV411())
        env->ThrowError("FCBI: input clip is unsupported format");
    if (vi.width < 16 || vi.height < 16)
        env->ThrowError("FCBI: input clip is too small");

    const int peak{ (1 << vi.BitsPerComponent()) - 1 };

    if (tm == -1)
        tm = (vi.ComponentSize() == 1) ? 30 : (30 * peak / 255);
    if (tm < 0 || tm > peak)
        env->ThrowError("FCBI: tm is out of range");
    if (opt < -1 || opt > 1)
        env->ThrowError("FCBI: opt must be between -1..1.");

    const int iset{ instrset_detect() };
    if (opt == 1 && iset < 2)
        env->ThrowError("FCBI: opt=1 requires SSE2.");

    vi.width *= 2;
    vi.height *= 2;

    vit = vi;

    if (vi.ComponentSize() == 1)
    {
        vit.pixel_type = VideoInfo::CS_Y8;
        process_phase1 = ((opt == -1 && iset >= 2) || opt == 1) ? phase1_sse2<uint8_t> : phase1_c<uint8_t>;

        if (_e)
        {
            process_phase2 = phase2_c<uint8_t, true>;
            process_phase3 = phase3_c<uint8_t, true>;
        }
        else
        {
            process_phase2 = phase2_c<uint8_t, false>;
            process_phase3 = phase3_c<uint8_t, false>;
        }
    }
    else
    {
        switch (vi.BitsPerComponent())
        {
            case 10: vit.pixel_type = VideoInfo::CS_Y10; break;
            case 12: vit.pixel_type = VideoInfo::CS_Y12; break;
            case 14: vit.pixel_type = VideoInfo::CS_Y14; break;
            default: vit.pixel_type = VideoInfo::CS_Y16;
        }

        process_phase1 = ((opt == -1 && iset >= 2) || opt == 1) ? phase1_sse2<uint16_t> : phase1_c<uint16_t>;

        if (_e)
        {
            process_phase2 = phase2_c<uint16_t, true>;
            process_phase3 = phase3_c<uint16_t, true>;
        }
        else
        {
            process_phase2 = phase2_c<uint16_t, false>;
            process_phase3 = phase3_c<uint16_t, false>;
        }
    }

    vit.width = (vit.width + 4 + 31) & ~31;
    vit.height += 2;

    try { env->CheckVersion(8); }
    catch (const AvisynthError&) { v8 = false; }
}

PVideoFrame __stdcall FCBI::GetFrame(int n, IScriptEnvironment* env)
{
    const int planes[3]{ PLANAR_Y, PLANAR_U, PLANAR_V };

    PVideoFrame src{ child->GetFrame(n, env) };
    PVideoFrame tmp{ env->NewVideoFrame(vit) };
    PVideoFrame dst{ (v8) ? env->NewVideoFrameP(vi, &src) : env->NewVideoFrame(vi) };

    const int tpitch{ tmp->GetPitch() };
    uint8_t* __restrict tmpp{ tmp->GetWritePtr() + tpitch };

    for (int p{ 0 }; p < vi.NumComponents(); ++p)
    {
        const int width{ dst->GetRowSize(planes[p]) };
        const int height{ dst->GetHeight(planes[p]) };

        process_phase1(src->GetReadPtr(planes[p]), tmpp, src->GetRowSize(planes[p]), src->GetHeight(planes[p]), src->GetPitch(planes[p]), tpitch);
        process_phase2(tmpp, width, height, tpitch, tm);
        process_phase3(tmpp, width, height, tpitch, tm);

        env->BitBlt(reinterpret_cast<uint8_t*>(dst->GetWritePtr(planes[p])), dst->GetPitch(planes[p]), reinterpret_cast<uint8_t*>(tmpp), tpitch, width, height);
    }

    return dst;
}

AVSValue __cdecl FCBI_create(AVSValue args, void*, IScriptEnvironment* env)
{
    enum opt { CLIP, ED, TM, OPT };

    return new FCBI(args[CLIP].AsClip(), args[ED].AsBool(false), args[TM].AsInt(-1), args[OPT].AsInt(-1), env);
}

const AVS_Linkage* AVS_linkage = nullptr;

extern "C" __declspec(dllexport) const char* __stdcall
AvisynthPluginInit3(IScriptEnvironment * env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;

    env->AddFunction("FCBI", "c[ed]b[tm]i[opt]i", FCBI_create, 0);
    return "FCBI for avisynth ver x.x.x";
}
