// Register-mapped HLSL reconstructed from this material's preserved FXO template.
// RGB colour clamps lift only with Juiced FP16 active (c223.x); scalar masks stay native.
float4 c4:register(c4);
float4 c24:register(c24);
float4 c25:register(c25);
float4 c26:register(c26);
float4 JuicedColour:register(c223);
sampler2D s0:register(s0);
sampler2D s1:register(s1);
float4 Select4(float4 t,float4 a,float4 b) { return float4(t.x>=0?a.x:b.x,t.y>=0?a.y:b.y,t.z>=0?a.z:b.z,t.w>=0?a.w:b.w); }
struct Inputs {
    float4 v0:TEXCOORD0;
};
float4 PS_Main(Inputs input):COLOR0 {
    float4 v0=input.v0;
    const float4 c0=float4(0.5,0.0,16.0,-1.0);
    const float4 c1=float4(2.0,-1.0,-2.0,1.0);
    float4 r0=0;
    float4 r1=0;
    float4 colour=0;
    r0=(tex2Dlod(s1,c0.xxyz)).xyzw;
    r0.x=((r0.wwww+c0.wwww)).x;
    r1=(tex2D(s1,(v0.xyzw).xy)).xyzw;
    r0.xy=(Select4(-abs(r0.xxxx),r1.yxzw,r1.wyzw)).xy;
    r0.yz=((r0.xxyw*c1.xxxx+c1.yyyy)).yz;
    r0.w=(float4(dot((r0.zyzw).xy,(-r0.zyzw).xy)+(-c0.wwww).x,dot((r0.zyzw).xy,(-r0.zyzw).xy)+(-c0.wwww).x,dot((r0.zyzw).xy,(-r0.zyzw).xy)+(-c0.wwww).x,dot((r0.zyzw).xy,(-r0.zyzw).xy)+(-c0.wwww).x)).w;
    r0.w=(rsqrt(abs(r0.wwww))).w;
    r0.x=((1/r0.wwww)).x;
    r0.x=(float4(dot((r0.yzxw).xyz,(c25.xyzw).xyz),dot((r0.yzxw).xyz,(c25.xyzw).xyz),dot((r0.yzxw).xyz,(c25.xyzw).xyz),dot((r0.yzxw).xyz,(c25.xyzw).xyz))).x;
    r0.y=(saturate(r0.xxxx)).y;
    r0.y=((-r0.yyyy+-c0.wwww)).y;
    r1=(tex2D(s0,(v0.xyzw).xy)).xyzw;
    r0.xzw=(saturate((r0.xxxx*r1.xyyz))).xzw;
    r1.xyz=((r1.xyzw*c24.xxxx)).xyz;
    r1.w=((r1.wwww*c26.wwww)).w;
    r1.w=((r1.wwww*c4.wwww)).w;
    r0.xyz=((r1.xyzw*r0.yyyy+r0.xzww)).xyz;
    r0.xyz=((r0.xyzw+-c0.xxxx)).xyz;
    r1.x=(c0.xxxx).x;
    r0.xyz=((r0.xyzw*c24.yyyy+r1.xxxx)).xyz;
    r0.xyz=((r0.xyzw*c24.zzzz)).xyz;
    r0.xyz=((JuicedColour.x>.5 ? max((r0.xyzw*c26.xyzw),0) : saturate((r0.xyzw*c26.xyzw)))).xyz;
    r0.xyz=((r0.xyzw*c1.zzzz+c1.wwww)).xyz;
    r0.xyz=((-r0.xyzw+-c0.wwww)).xyz;
    r0.w=(max(r1.wwww,c4.xxxx)).w;
    colour.w=(r1.wwww).w;
    colour.xyz=((r0.xyzw*r0.wwww)).xyz;
    return colour;
}
