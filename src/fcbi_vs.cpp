#include <memory>
#include <string>

#include "fcbi.h"
#include "VapourSynth4.h"
#include "VSHelper4.h"
#include "VCL2/instrset.h"


using namespace std::literals;

struct FCBIData
{
    VSNode* node;
    VSVideoInfo vi;

    int tm;
    VSVideoInfo vit;

    void (*process_phase1)(const uint8_t* srcp, uint8_t* __restrict dstp, const int width, const int height, const int spitch, const int dpitch) noexcept;
    void (*process_phase2)(uint8_t* __restrict ptr, const int width, const int height, const int pitch, const int tm) noexcept;
    void (*process_phase3)(uint8_t* __restrict ptr, const int width, const int height, const int pitch, const int tm) noexcept;
};

static const VSFrame* VS_CC FCBIGetFrame(int n, int activationReason, void* instanceData, [[maybe_unused]] void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi)
{
    FCBIData* d{ static_cast<FCBIData*>(instanceData) };

    if (activationReason == arInitial)
        vsapi->requestFrameFilter(n, d->node, frameCtx);
    else if (activationReason == arAllFramesReady)
    {
        const VSFrame* src{ vsapi->getFrameFilter(n, d->node, frameCtx) };
        VSFrame* tmp{ vsapi->newVideoFrame(&d->vit.format, d->vit.width, d->vit.height, src, core) };
        VSFrame* dst{ vsapi->newVideoFrame(&d->vi.format, d->vi.width, d->vi.height, src, core) };

        const ptrdiff_t tpitch{ vsapi->getStride(tmp, 0) };
        uint8_t* __restrict tmpp{ vsapi->getWritePtr(tmp, 0) + tpitch };

        for (int p{ 0 }; p < d->vi.format.numPlanes; ++p)
        {
            const int width{ vsapi->getFrameWidth(dst, p) };
            const int height{ vsapi->getFrameHeight(dst, p) };

            d->process_phase1(vsapi->getReadPtr(src, p), tmpp, vsapi->getFrameWidth(src, p), vsapi->getFrameHeight(src, p), vsapi->getStride(src, p), tpitch);
            d->process_phase2(tmpp, width, height, tpitch, d->tm);
            d->process_phase3(tmpp, width, height, tpitch, d->tm);

            vsh::bitblt(reinterpret_cast<uint8_t*>(vsapi->getWritePtr(dst, p)), vsapi->getStride(dst, p), reinterpret_cast<uint8_t*>(tmpp), tpitch, width * d->vi.format.bytesPerSample, height);
        }

        vsapi->freeFrame(src);
        vsapi->freeFrame(tmp);

        return dst;
    }

    return nullptr;
}

static void VS_CC FCBIFree(void* instanceData, [[maybe_unused]] VSCore* core, const VSAPI* vsapi) {
    FCBIData* d{ static_cast<FCBIData*>(instanceData) };
    vsapi->freeNode(d->node);
    delete d;
}

static void VS_CC FCBICreate(const VSMap* in, VSMap* out, [[maybe_unused]] void* userData, VSCore* core, const VSAPI* vsapi)
{
    std::unique_ptr<FCBIData> d{ std::make_unique<FCBIData>() };

    try
    {
        d->node = vsapi->mapGetNode(in, "clip", 0, nullptr);
        d->vi = *vsapi->getVideoInfo(d->node);
        int err{ 0 };

        if (d->vi.format.colorFamily == cfRGB || d->vi.format.sampleType == stFloat || d->vi.format.bytesPerSample == 4)
            throw "clip must be in YUV 8..16-bit planar format."s;
        if (d->vi.format.subSamplingW > 1)
            throw "input clip is unsupported format."s;
        if (d->vi.width < 16 || d->vi.height < 16)
            throw "input clip is too small."s;

        const int peak{ (1 << d->vi.format.bitsPerSample) - 1 };

        d->tm = vsapi->mapGetIntSaturated(in, "tm", 0, &err);
        if (err)
            d->tm = (d->vi.format.bytesPerSample == 1) ? 30 : (30 * peak / 255);
        if (d->tm < 0 || d->tm > peak)
            throw "tm is out of range."s;

        int64_t opt{ vsapi->mapGetInt(in, "opt", 0, &err) };
        if (err)
            opt = -1;
        if (opt < -1 || opt > 1)
            throw "opt must be between -1..1."s;

        const int iset{ instrset_detect() };
        if (opt == 1 && iset < 2)
            throw "opt = 1 requires SSE2."s;

        d->vi.width *= 2;
        d->vi.height *= 2;

        d->vit = d->vi;
        d->vit.format.colorFamily = cfGray;

        const bool _e{ !!vsapi->mapGetIntSaturated(in, "ed", 0, &err) };

        if (d->vi.format.bytesPerSample == 1)
        {
            d->process_phase1 = ((opt == -1 && iset >= 2) || opt == 1) ? phase1_sse2<uint8_t> : phase1_c<uint8_t>;

            if (_e)
            {
                d->process_phase2 = phase2_c<uint8_t, true>;
                d->process_phase3 = phase3_c<uint8_t, true>;
            }
            else
            {
                d->process_phase2 = phase2_c<uint8_t, false>;
                d->process_phase3 = phase3_c<uint8_t, false>;
            }
        }
        else
        {
            d->process_phase1 = ((opt == -1 && iset >= 2) || opt == 1) ? phase1_sse2<uint16_t> : phase1_c<uint16_t>;

            if (_e)
            {
                d->process_phase2 = phase2_c<uint16_t, true>;
                d->process_phase3 = phase3_c<uint16_t, true>;
            }
            else
            {
                d->process_phase2 = phase2_c<uint16_t, false>;
                d->process_phase3 = phase3_c<uint16_t, false>;
            }
        }

        d->vit.width = (d->vit.width + 4 + 31) & ~31;
        d->vit.height += 2;
    }
    catch (const std::string& error)
    {
        vsapi->mapSetError(out, ("grayworld: " + error).c_str());
        vsapi->freeNode(d->node);
        return;
    }

    VSFilterDependency deps[] = { {d->node, rpStrictSpatial} };
    vsapi->createVideoFilter(out, "FCBI", &d->vi, FCBIGetFrame, FCBIFree, fmParallel, deps, 1, d.get(), core);
    d.release();
}

VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin* plugin, const VSPLUGINAPI* vspapi)
{
    vspapi->configPlugin("com.vapoursynth.fcbi", "fcbi", "Fast Curvature Based Interpolation.", VS_MAKE_VERSION(1, 0), VAPOURSYNTH_API_VERSION, 0, plugin);
    vspapi->registerFunction("FCBI",
        "clip:vnode;"
        "ed:int:opt;"
        "tm:int:opt;"
        "opt:int:opt;",
        "clip:vnode;",
        FCBICreate, nullptr, plugin);
}
