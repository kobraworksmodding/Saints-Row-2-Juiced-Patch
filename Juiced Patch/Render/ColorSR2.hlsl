// FP16 colour pipeline. SR2's legacy material/postFX output is gamma encoded.
// Decode once for display mapping, then preserve the game's colour treatment.
sampler2D Scene : register(s0);
float4 Output : register(c0); // exposure EV, paper white nits, peak nits, HDR
float4 Controls : register(c1); // SDR shoulder start, HDR rolloff, dither, debug
float Luma(float3 c) { return dot(c,float3(.2126,.7152,.0722)); }
float3 Decode(float3 c) { return pow(max(c,0),2.2); }
float3 Encode(float3 c) { return pow(max(c,0),1.0/2.2); }
float3 Shoulder(float3 c,float peak,float start) {
    float brightest=max(c.x,max(c.y,c.z));
    if (brightest>start) {
        float range=max(peak-start,1e-5);
        float mapped=start+range*(1-exp(-(brightest-start)/range));
        c*=mapped/max(brightest,1e-5);
    }
    return c;
}
float4 PS_Tonemap(float2 uv:TEXCOORD0):COLOR0 {
    float4 scene=tex2Dlod(Scene,float4(uv,0,0));
    if (Controls.w>.5) {
        float3 c=scene.rgb;
        return float4(max(c.x,max(c.y,c.z))>1 ? float3(1,0,.5) : c*.25,1);
    }
    float3 sceneLinear=Decode(scene.rgb)*exp2(Output.x);
    float peak=Output.w>.5 ? max(Output.z/Output.y,1.001) : 1;
    float start=Output.w>.5 ? min(peak*Controls.y,1) : Controls.x;
    return float4(Encode(Shoulder(sceneLinear,peak,start)),scene.a);
}
// Final pass is AFTER UI. Tone mapping is already complete, so UI is unaffected.
// HDR uses FusionFix's scRGB convention: linear 1.0 equals 80 nits.
float4 PS_Output(float2 uv:TEXCOORD0,float2 pixel:VPOS):COLOR0 {
    float3 c=max(tex2Dlod(Scene,float4(uv,0,0)).rgb,0);
    if (Output.w>.5) return float4(Decode(c)*(Output.y/80),1);
    // Static, sub-LSB triangular noise, in output gamma space. No blur or shimmer.
    float a=frac(52.9829189*frac(dot(pixel,float2(.06711056,.00583715))));
    float b=frac(52.9829189*frac(dot(pixel+float2(37,17),float2(.06711056,.00583715))));
    return float4(saturate(c+(a-b)*(Controls.z/255)),1);
}
