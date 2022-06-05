#pragma once

#include <cstring>
#include <type_traits>

#include "avisynth.h"

class FCBI : public GenericVideoFilter
{
    const int tm;
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

template <typename T>
void phase1_sse2(const uint8_t* srcp_, uint8_t* __restrict dstp_, int width, const int height, int spitch, int dpitch) noexcept;
