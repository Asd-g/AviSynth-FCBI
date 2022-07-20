#pragma once

#include <cstdint>
#include <type_traits>

template <typename T>
void phase1_c(const uint8_t* srcp_, uint8_t* __restrict dstp_, int width, const int height, int spitch, int dpitch) noexcept;
template <typename T>
void phase1_sse2(const uint8_t* srcp_, uint8_t* __restrict dstp_, int width, const int height, int spitch, int dpitch) noexcept;

template <typename T, bool EDGE>
void phase2_c(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept;
template <typename T, bool EDGE>
void phase3_c(uint8_t* __restrict ptr, int width, const int height, int pitch, const int tm) noexcept;
