// Reconstructed from Juiced's shipped distortion shader registers and math.
// FP16-safe gamma extension, retaining SDR gamma, distortion, BCS and dither filtering.
float4 Tint:register(c4);
float3 BCS:register(c7);
float2 max_distortion_offset:register(c24);
float4 fullscreen_distortion_strength:register(c25);
float Bleach_bypass_strength:register(c26);
float4 distortion_juicedsettings:register(c187);
float4 screenRes:register(c188);
float4 AlphaMask:register(c189);
sampler2D base_sampler:register(s0);
sampler2D distortion_sampler:register(s1);
sampler2D fullscreen_distortion_sampler:register(s2);
float Luma(float3 c) { return dot(c,float3(.2126,.7152,.0722)); }
float3 ConsoleGamma(float3 c) {
    // The existing curve is accurate in [0,1]. Bound only its exponential
    // argument outside that domain; highlights retain an unbounded power curve.
    c=clamp(c,0,10000);
    float3 high=pow(c,1.13464999)*(1-exp2(-13.5305405*min(c,1)));
    return lerp(c*.541900992,high,saturate(c*6.56649017+.311464995));
}
float4 PS_Distortion(float2 uv:TEXCOORD0,float4 fullscreenUV:TEXCOORD1):COLOR0 {
    float4 dist=tex2D(distortion_sampler,uv);
    float4 full1=tex2D(fullscreen_distortion_sampler,fullscreenUV.xy);
    float4 full2=tex2D(fullscreen_distortion_sampler,fullscreenUV.zw);
    float2 offset=(dist.xy*2-1.00392163)*max_distortion_offset;
    offset+=((full1.xy*2-1.00392163)+(full2.xy*2-1.00392163))*fullscreen_distortion_strength.xy;
    uv+=offset;
    float strength=saturate(dot(full1.zw*full2.wz,float2(1,1))*fullscreen_distortion_strength.z);
    float4 c=tex2D(base_sampler,uv);
    if (AlphaMask.x==0 && AlphaMask.y==1) {
        float2 t=1/screenRes.xy;
        float3 a=tex2D(base_sampler,uv+t*float2(-.5,-1.5)).rgb;
        float3 b=tex2D(base_sampler,uv+t*float2(1.5,-.5)).rgb;
        float3 d=tex2D(base_sampler,uv+t*float2(.5,1.5)).rgb;
        float3 e=tex2D(base_sampler,uv+t*float2(-1.5,.5)).rgb;
        float4 l=float4(Luma(a),Luma(b),Luma(d),Luma(e));
        float mean=dot(l,float4(.25,.25,.25,.25));
        float delta=Luma(c.rgb)-mean;
        float4 differences=l-mean;
        float weight=saturate((delta*delta-dot(differences,differences)+.008)*20);
        c=float4(lerp(c.rgb,(a+b+d+e)*.25,weight),1);
    }
    float3 sceneLinear=pow(max(c.rgb,0),2.2);
    c.rgb=pow(max(lerp(sceneLinear,Luma(sceneLinear),strength),0),1.0/2.2);
    float luma=Luma(c.rgb);
    float3 multiply=2*c.rgb*luma;
    float3 screen=1-2*(1-luma)*(1-c.rgb);
    float3 bleach=lerp(multiply,screen,saturate((luma-.45)*10));
    c.rgb=lerp(c.rgb,bleach,Bleach_bypass_strength);
    c.rgb=lerp(Luma(c.rgb),c.rgb,BCS.z);
    c=(c*BCS.y+.5*(1-BCS.y)+BCS.x)*Tint;
    if (distortion_juicedsettings.x==0) c.rgb=ConsoleGamma(c.rgb);
    return c;
}
