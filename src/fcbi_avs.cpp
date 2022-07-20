#include "avisynth.h"
#include "fcbi.h"
#include "VCL2/instrset.h"

class FCBI : public GenericVideoFilter
{
    int tm;
    VideoInfo vit;
    bool v8;

    void (*process_phase1)(const uint8_t* srcp, uint8_t* __restrict dstp, const int width, const int height, const int spitch, const int dpitch) noexcept;
    void (*process_phase2)(uint8_t* __restrict ptr, const int width, const int height, const int pitch, const int tm) noexcept;
    void (*process_phase3)(uint8_t* __restrict ptr, const int width, const int height, const int pitch, const int tm) noexcept;

public:
    FCBI(PClip child, bool edge, int tm, int opt, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;

    int __stdcall SetCacheHints(int hints, int) override
    {
        return hints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
    }
};

FCBI::FCBI(PClip _c, bool _e, int _t, int opt, IScriptEnvironment* env)
    : GenericVideoFilter(_c), tm(_t), v8(true)
{
    if (!vi.IsPlanar() || vi.IsRGB())
        env->ThrowError("FCBI: input clip is not planar YUV format.");
    if (vi.ComponentSize() == 4)
        env->ThrowError("FCBI: bit depth of input clip is must be 8..16-bit.");
    if (vi.NumComponents() == 4)
        env->ThrowError("FCBI: alpha is unsupported.");
    if (vi.IsYV411())
        env->ThrowError("FCBI: input clip is unsupported format.");
    if (vi.width < 16 || vi.height < 16)
        env->ThrowError("FCBI: input clip is too small.");

    const int peak{ (1 << vi.BitsPerComponent()) - 1 };

    if (tm == -1)
        tm = (vi.ComponentSize() == 1) ? 30 : (30 * peak / 255);
    if (tm < 0 || tm > peak)
        env->ThrowError("FCBI: tm is out of range.");
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
        const int width{ dst->GetRowSize(planes[p]) / vi.ComponentSize() };
        const int height{ dst->GetHeight(planes[p]) };

        process_phase1(src->GetReadPtr(planes[p]), tmpp, src->GetRowSize(planes[p]) / vi.ComponentSize(), src->GetHeight(planes[p]), src->GetPitch(planes[p]), tpitch);
        process_phase2(tmpp, width, height, tpitch, tm);
        process_phase3(tmpp, width, height, tpitch, tm);

        env->BitBlt(reinterpret_cast<uint8_t*>(dst->GetWritePtr(planes[p])), dst->GetPitch(planes[p]), reinterpret_cast<uint8_t*>(tmpp), tpitch, dst->GetRowSize(planes[p]), height);
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
