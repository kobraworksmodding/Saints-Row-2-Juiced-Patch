#pragma once

// Build switches: 0 = exclude, 1 = include. Rebuild after changing these.
// JUICED_TAA=0 excludes all TAA, regardless of JUICED_TAA_MSAA.
#ifndef JUICED_TAA
#define JUICED_TAA 1
#endif
#ifndef JUICED_TAA_MSAA
#define JUICED_TAA_MSAA 0
#endif

#if (JUICED_TAA != 0 && JUICED_TAA != 1) || (JUICED_TAA_MSAA != 0 && JUICED_TAA_MSAA != 1)
#error TAA build switches must be 0 or 1.
#endif

namespace TemporalAA
{
#if JUICED_TAA
    void Init();
    // TAA replaces the separate native dither smoothing filter while active.
    bool UsesTemporalFiltering();
    // Release before the native render-target teardown and device Reset.
    void ReleaseResources();
#else
    inline void Init() {}
    inline bool UsesTemporalFiltering() { return false; }
    inline void ReleaseResources() {}
#endif
}
