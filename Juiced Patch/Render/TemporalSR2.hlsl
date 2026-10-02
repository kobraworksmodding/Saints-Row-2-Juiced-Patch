// SR2-specific inputs for FusionFix's in-process D3D9 temporal resolve.
// The resolve below is derived from GTAIV.EFLC.FusionFix Temporal.hlsl (GPL-3.0).
// SR2 uses perspective depth, c4-c7 MVPs and c52-c243 bones, not GTA IV's log depth.
sampler2D SceneDepth : register(s0);
float4 gCameraJitter : register(c0); // NDC jitter xy; half-texel correction zw.
float4 gCameraDepth : register(c5); // native projection [10], [14]
row_major float4x4 gCameraReproject : register(c1); // inverse current VP * previous VP.
float4 PS_CameraMotion(float2 uv : TEXCOORD0) : COLOR0
{
    float z = tex2Dlod(SceneDepth, float4(uv, 0, 0)).r;
    float2 ndc = float2(uv.x * 2 - 1, 1 - uv.y * 2) - gCameraJitter.xy - gCameraJitter.zw;
    float4 previous = mul(float4(ndc, z, 1), gCameraReproject);
    float2 motion = (previous.xy / previous.w - ndc) * float2(0.5, -0.5);
    float currentW = max(gCameraDepth.y / min(z - gCameraDepth.x, -1e-7), 1e-5);
    return float4(motion, previous.w > 0, max(previous.w * currentW, 1e-5));
}
float4 PS_CopyDepth(float2 uv : TEXCOORD0) : COLOR0
{
    return tex2Dlod(SceneDepth, float4(uv, 0, 0)).rrrr;
}
float4 gVelocityParams : register(c0); // inverse width, inverse height, history valid, depth tolerance.
float4 PS_Velocity(float4 current : TEXCOORD0, float4 previous : TEXCOORD1, float2 pixel : VPOS) : COLOR0
{
    float z = tex2Dlod(SceneDepth, float4((pixel + 0.5) * gVelocityParams.xy, 0, 0)).r;
    float centerDepth = current.z / current.w;
    // RESZ supplies an actual MSAA sample, which can differ from pixel-center
    // depth on a slope. Bound that difference by half a pixel's depth gradient.
    // Positive tolerance retains the exact R5/R6 single-sample matching path.
    float tolerance = abs(gVelocityParams.w);
    if (gVelocityParams.w < 0)
        tolerance += 0.5 * (abs(ddx(centerDepth)) + abs(ddy(centerDepth)));
    clip(tolerance - abs(z - centerDepth));
    float2 motion = (previous.xy / previous.w - current.xy / current.w) * float2(0.5, -0.5);
    return float4(motion, gVelocityParams.z * (previous.w > 0), max(previous.w, 1e-5));
}
sampler2D SceneTex : register(s0);
sampler2D OpaqueTex : register(s1);
float Luma(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }
float4 PS_OpaqueLuma(float2 uv : TEXCOORD0) : COLOR0
{
    return Luma(tex2Dlod(SceneTex, float4(uv, 0, 0)).rgb).xxxx;
}
float4 gCopyAlpha : register(c0); // temporal scene alpha enabled
float4 PS_CopyColor(float2 uv : TEXCOORD0) : COLOR0
{
    // HDR reduction weights scene color with alpha. Emissive glow is a
    // separate native texture, resolved independently before its blur/composite.
    float4 resolved = tex2Dlod(SceneTex, float4(uv, 0, 0));
    if (gCopyAlpha.x < 0.5) resolved.a = tex2Dlod(OpaqueTex, float4(uv, 0, 0)).a;
    return resolved;
}
float ReactiveValue(float scene, float opaque, float4 params)
{
    scene = max(scene, 0.0);
    opaque = max(opaque, 0.0);
    float difference = abs(scene / (1.0 + scene) - opaque / (1.0 + opaque));
    return saturate(difference * params.x) * params.y;
}

// FusionFix temporal resolve follows, with per-pixel history rejection and a
// single floating-point history output (SR2 does not need mixed-format MRTs).
// RGBA history preserves native scene alpha. Log-depth lives in a separate
// scalar history target so color alpha never doubles as temporal metadata.
sampler2D CurrentTex  : register(s0);
sampler2D HistoryTex  : register(s1);
sampler2D MotionTex   : register(s2);
sampler2D DepthTex    : register(s3); // final standard [0, 1] main-scene depth
sampler2D OpaqueLumaTex : register(s4); // see the reactive mask
sampler2D HistoryDepthTex : register(s5); // separate log2 clip-w history

float4 gTexel   : register(c0); // 1 / width, 1 / height, width, height
float4 gTaa     : register(c1); // xy: jitter in pixels, z: history valid, w: variance clip gamma
float4 gBlend   : register(c2); // x: minimum current weight, y: maximum current weight, z: weight per pixel of motion, w: luma weight
float4 gResolveReactive : register(c3); // x: scale, y: maximum current weight, z: enabled
float4 gDepthReject : register(c4); // projection [10], [14], log-depth tolerance, enabled


// A snapped neighborhood bounds the previous surface's depth. Bilinear
// interpolation across a foreground/background boundary is not a surface depth,
// and a single dilated sample can change with the subpixel jitter on a slope.
bool HistoryDepthMatches(sampler2D historyDepth, float2 uv, float previousW, float2 texel, float4 params)
{
    float2 center = (floor(uv / texel) + 0.5) * texel;
    float lo = tex2Dlod(historyDepth, float4(center, 0, 0)).r;
    float hi = lo;
    float d = tex2Dlod(historyDepth, float4(center + float2(texel.x, 0), 0, 0)).r;
    lo = min(lo, d); hi = max(hi, d);
    d = tex2Dlod(historyDepth, float4(center - float2(texel.x, 0), 0, 0)).r;
    lo = min(lo, d); hi = max(hi, d);
    d = tex2Dlod(historyDepth, float4(center + float2(0, texel.y), 0, 0)).r;
    lo = min(lo, d); hi = max(hi, d);
    d = tex2Dlod(historyDepth, float4(center - float2(0, texel.y), 0, 0)).r;
    lo = min(lo, d); hi = max(hi, d);
    float expected = log2(max(previousW, 1e-5));
    return params.w < 0.5 || (expected >= lo - params.z && expected <= hi + params.z);
}

float NearestDepth(sampler2D depth, float2 uv, float2 texel)
{
    float nearest = 1;
    [unroll] for (int y=-1; y<=1; ++y)
        [unroll] for (int x=-1; x<=1; ++x)
            nearest = min(nearest, tex2Dlod(depth, float4(uv + float2(x,y)*texel, 0, 0)).r);
    return nearest;
}
float4 gStoreDepth : register(c0); // projection [10], [14]
float4 gStoreTexel : register(c1);
float4 PS_StoreHistoryDepth(float2 uv : TEXCOORD0) : COLOR0
{
    float depth = NearestDepth(SceneDepth, uv, gStoreTexel.xy);
    float w = max(gStoreDepth.y / min(depth - gStoreDepth.x, -1e-7), 1e-5);
    return log2(w).xxxx;
}

// Luminance weighted reversible tonemap: keeps bright samples from dominating the filters
float3 Compress(float3 c)
{
    return c / (1.0 + Luma(c) * gBlend.w);
}

float3 Expand(float3 c)
{
    return c / max(1.0 - Luma(c) * gBlend.w, 1.0 / 65504.0);
}

float3 ToYCoCg(float3 c)
{
    return float3(dot(c, float3(0.25, 0.5, 0.25)), dot(c, float3(0.5, 0.0, -0.5)), dot(c, float3(-0.25, 0.5, -0.25)));
}

float3 FromYCoCg(float3 c)
{
    return float3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}

float3 FetchCurrent(float2 uv)
{
    float3 c = tex2Dlod(CurrentTex, float4(uv, 0.0, 0.0)).rgb;
    return ToYCoCg(Compress(clamp(c, 0.0, 65000.0)));
}

float3 FetchHistory(float2 uv)
{
    // Catmull-Rom through five bilinear taps
    float2 position = uv * gTexel.zw;
    float2 center = floor(position - 0.5) + 0.5;
    float2 f = position - center;

    float2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    float2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    float2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    float2 w3 = f * f * (-0.5 + 0.5 * f);

    float2 w12 = w1 + w2;
    float2 tc12 = (center + w2 / w12) * gTexel.xy;
    float2 tc0 = (center - 1.0) * gTexel.xy;
    float2 tc3 = (center + 2.0) * gTexel.xy;

    float4 result = float4(tex2Dlod(HistoryTex, float4(tc12.x, tc0.y, 0.0, 0.0)).rgb, 1.0) * (w12.x * w0.y);
    result += float4(tex2Dlod(HistoryTex, float4(tc0.x, tc12.y, 0.0, 0.0)).rgb, 1.0) * (w0.x * w12.y);
    result += float4(tex2Dlod(HistoryTex, float4(tc12.x, tc12.y, 0.0, 0.0)).rgb, 1.0) * (w12.x * w12.y);
    result += float4(tex2Dlod(HistoryTex, float4(tc3.x, tc12.y, 0.0, 0.0)).rgb, 1.0) * (w3.x * w12.y);
    result += float4(tex2Dlod(HistoryTex, float4(tc12.x, tc3.y, 0.0, 0.0)).rgb, 1.0) * (w12.x * w3.y);

    return ToYCoCg(Compress(clamp(result.rgb / result.a, 0.0, 65000.0)));
}

float3 ClipTowardsCenter(float3 boxMin, float3 boxMax, float3 history)
{
    float3 center = 0.5 * (boxMax + boxMin);
    float3 extents = 0.5 * (boxMax - boxMin) + 0.00001;
    float3 offset = history - center;
    float3 units = abs(offset / extents);
    float maxUnit = max(units.x, max(units.y, units.z));
    return maxUnit > 1.0 ? center + offset / maxUnit : history;
}

float4 PS_TemporalResolve(float2 uv : TEXCOORD0) : COLOR0
{
    static const float2 offsets[9] =
    {
        float2(-1.0, -1.0), float2(0.0, -1.0), float2(1.0, -1.0),
        float2(-1.0,  0.0), float2(0.0,  0.0), float2(1.0,  0.0),
        float2(-1.0,  1.0), float2(0.0,  1.0), float2(1.0,  1.0)
    };

    float3 m1 = 0.0;
    float3 m2 = 0.0;
    float3 boxMin = 65504.0;
    float3 boxMax = -65504.0;
    float3 filtered = 0.0;
    float filterWeight = 0.0;
    float bloomFiltered = 0, bloomMin = 65000, bloomMax = 0, bloomM1 = 0, bloomM2 = 0;

    float closestDepth = 1.0;
    float2 closestOffset = 0.0;

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        float2 tap = uv + offsets[i] * gTexel.xy;
        float4 native = tex2Dlod(CurrentTex, float4(tap, 0, 0));
        float3 c = ToYCoCg(Compress(clamp(native.rgb, 0, 65000)));
        float bloom = clamp(native.a, 0, 65000);
        bloomMin = min(bloomMin, bloom); bloomMax = max(bloomMax, bloom);
        bloomM1 += bloom; bloomM2 += bloom*bloom;

        m1 += c;
        m2 += c * c;
        boxMin = min(boxMin, c);
        boxMax = max(boxMax, c);

        // Each pixel holds the scene sampled at its center minus the jitter
        float2 d = offsets[i] - gTaa.xy;
        float w = exp(-2.29 * dot(d, d));
        filtered += c * w;
        bloomFiltered += bloom * w;
        filterWeight += w;


        float depth = tex2Dlod(DepthTex, float4(tap, 0.0, 0.0)).r;
        if (depth < closestDepth)
        {
            closestDepth = depth;
            closestOffset = offsets[i];
        }
    }

    filtered /= filterWeight;
    bloomFiltered /= filterWeight;

    float4 motionInfo = tex2Dlod(MotionTex, float4(uv + closestOffset * gTexel.xy, 0.0, 0.0));
    float2 motion = motionInfo.xy;
    float2 historyUV = uv + motion;

    float3 mean = m1 / 9.0;
    float3 sigma = sqrt(max(m2 / 9.0 - mean * mean, 0.0));
    float3 clipMin = max(boxMin, mean - gTaa.w * sigma);
    float3 clipMax = min(boxMax, mean + gTaa.w * sigma);

    float3 result = filtered;
    bool onScreen = all(historyUV == saturate(historyUV));

    bool depthMatches = HistoryDepthMatches(HistoryDepthTex, historyUV, motionInfo.w, gTexel.xy, gDepthReject);
    float bloomResult = bloomFiltered;
    if (gTaa.z > 0.0 && onScreen && motionInfo.z > 0.5 && depthMatches)
    {
        float3 history = ClipTowardsCenter(clipMin, clipMax, FetchHistory(historyUV));

        float speed = length(motion * gTexel.zw);
        float currentWeight = lerp(gBlend.x, gBlend.y, saturate(speed * gBlend.z));

        if (gResolveReactive.z > 0.0)
        {
            float scene = Luma(tex2Dlod(CurrentTex, float4(uv, 0.0, 0.0)).rgb);
            float opaque = tex2Dlod(OpaqueLumaTex, float4(uv, 0.0, 0.0)).r;
            currentWeight = max(currentWeight, ReactiveValue(scene, opaque, gResolveReactive));
        }

        result = lerp(history, filtered, currentWeight);
        float bloomMean = bloomM1/9;
        float bloomSigma = sqrt(max(bloomM2/9 - bloomMean*bloomMean, 0));
        float bloomLo = max(bloomMin, bloomMean - gTaa.w*bloomSigma);
        float bloomHi = min(bloomMax, bloomMean + gTaa.w*bloomSigma);
        float oldBloom = clamp(tex2Dlod(HistoryTex, float4(historyUV, 0, 0)).a, bloomLo, bloomHi);
        bloomResult = lerp(oldBloom, bloomFiltered, currentWeight);
    }

    return float4(Expand(FromYCoCg(result)), bloomResult);
}

// Debug is drawn after post-processing into the final target, never into history.
sampler2D DebugMotion : register(s0);
sampler2D DebugDepth : register(s1);
sampler2D DebugScene : register(s2);
sampler2D DebugOpaque : register(s3);
sampler2D DebugHistory : register(s4);
sampler2D DebugPreviousDepth : register(s5);
float4 gDebug : register(c0); // mode, motion scale in pixels, source width, source height
float4 gDebugReactive : register(c1);
float4 gDebugDepth : register(c2); // depth rejection params
float4 gDebugHistory : register(c3); // this frame reused history
float4 PS_Debug(float2 uv : TEXCOORD0) : COLOR0
{
    float3 motion = tex2Dlod(DebugMotion, float4(uv, 0, 0)).xyz;
    if (gDebug.x < 1.5)
        return float4(0.5 + 0.5 * clamp(motion.xy * gDebug.zw / gDebug.y, -1, 1), 0.5, 1);
    if (gDebug.x < 2.5)
        return float4(tex2Dlod(DebugDepth, float4(uv, 0, 0)).rrr, 1);
    if (gDebug.x < 3.5)
    {
        float scene = Luma(tex2Dlod(DebugScene, float4(uv, 0, 0)).rgb);
        float opaque = tex2Dlod(DebugOpaque, float4(uv, 0, 0)).r;
        float reactive = gDebugReactive.z * ReactiveValue(scene, opaque, gDebugReactive);
        return float4(reactive.xxx, 1);
    }
    if (gDebug.x < 4.5)
        return float4(tex2Dlod(DebugHistory, float4(uv, 0, 0)).rgb, 1);
    if (gDebug.x < 5.5)
        return float4(1 - motion.z, motion.z, 0, 1);
    if (gDebug.x < 6.5)
        return float4(tex2Dlod(DebugHistory, float4(uv,0,0)).rgb, 1);
    if (gDebug.x > 7.5)
        return float4(tex2Dlod(DebugHistory, float4(uv,0,0)).aaa, 1);
    float2 texel = 1/gDebug.zw;
    float closest = 1;
    float4 info = tex2Dlod(DebugMotion, float4(uv,0,0));
    [unroll] for (int y=-1; y<=1; ++y)
        [unroll] for (int x=-1; x<=1; ++x)
        {
            float2 tap = uv+float2(x,y)*texel;
            float depth = tex2Dlod(DebugDepth,float4(tap,0,0)).r;
            if (depth < closest) { closest=depth; info=tex2Dlod(DebugMotion,float4(tap,0,0)); }
        }
    float2 previousUV=uv+info.xy;
    bool reuse = gDebugHistory.x>0.5 && info.z>0.5 &&
        all(previousUV==saturate(previousUV)) &&
        HistoryDepthMatches(DebugPreviousDepth,previousUV,info.w,texel,gDebugDepth);
    return reuse ? float4(0,1,0,1) : float4(1,0,0,1);
}
