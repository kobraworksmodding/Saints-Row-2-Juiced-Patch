# Experimental FP16 colour pipeline

The Juiced renderer now has an optional RGBA16F colour path with a final display
mapping pass. It runs inside SR2's 32-bit process; it needs no 64-bit upscaler or
IPC helper. A compatible **DXVK D3D9 HDR interface** is required to use a floating
point backbuffer, even for SDR output. The installed DXVK was exercised by the GPU
fixture; native D3D9 falls back to the existing path.

The feature defaults off. Add this to the existing `juiced.ini` for the first SDR
test; keep the rest of your configuration:

```ini
[ColorPipeline]
FP16=1
HDR=0
OutputDither=1
ExposureEV=0
PaperWhiteNits=200
PeakBrightnessNits=1000
SDRShoulder=0.75
HDRRollOff=0.8
```

Restart after changing FP16. Other controls are live in **Juiced Colour / HDR** in
BlingMenu. `Active FP16` should be 1 and `Scene tone-map passes` should increase
while rendering gameplay. `Failed colour passes` should stay at 0. HDR additionally
requires Windows HDR enabled and a supported HDR display; `Active HDR` reports
whether the swapchain accepted scRGB. Set peak nits for the display and use paper
white to set the brightness of ordinary scene white and UI. Unsupported HDR uses
SDR mapping.

Colour targets promoted to RGBA16F include scene, emissive glow, bloom/postFX,
colour reflections and the final backbuffer, plus SR2's native MSAA colour surface.
Depth/stencil, distortion vectors, luminance/water data, AO and fixed RGBA8 player
image readbacks retain their native formats. The rendering command buffer carries
colour settings to the render thread. Scene tone mapping happens after native
postFX/upscaling and before HUD. The final output transfer happens after HUD.
State and render-target bindings are restored after both passes. Device reset
releases the new resources; unsupported device creation falls back to native SDR.

The mapper decodes SR2's legacy gamma-encoded colour, applies exposure and a
hue-preserving highlight shoulder, then encodes it for the game's UI composition.
SDR maps into [0,1]; HDR maps into display peak/paper-white range and transfers the
result to scRGB (linear 1.0 = 80 nits). This preserves SR2's existing lighting/colour
model; it is not a conversion of every material to physically linear lighting.

Rendering precision removes quantization/clipping imposed by the promoted RGBA8
targets. It cannot recover banding already in textures, lookup tables or baked
lighting. The SDR output still has the display's precision limit; optional static
sub-LSB noise distributes final quantization without blurring or temporal noise.
There is no spatial debanding filter in this implementation.

Shader changes are in both repositories. Juiced's embedded distortion/gamma
shader retains its SDR curve and extends highlight handling. Juiced loose shader
HLSL covers the moon, two window3 materials, both window4 variants and water3.
BlingGFX HLSL covers sun glow, skybox clouds and its four window variants. Relevant
RGB clamps lift only while `c223.x` is active; alpha, dither/depth coverage, fog
factors and scalar masks keep their existing bounds. The shader lookup order stays
**BlingGFX -> Juiced -> vanilla**. Both sets must be installed when using BlingGFX.

`JUICED_FP16` in `Render/ColorPipeline.h` compiles the new renderer in/out. Existing
`JUICED_TAA` and `JUICED_TAA_MSAA` switches are unchanged; FP16 native MSAA support
is independent of the optional TAA+MSAA switch. No hair shader rewrite is needed
for this colour feature.

Both Actions workflows compile HLSL with the native
[FXOCompiler](../tools/FXOCompiler/README.md); shader build failure stops packaging.
Templates, HLSL and manifests are checked in. Generated FXO sizes and untouched
programs/layout bytes are preserved. There is no Python shader build step.

Validation: Release builds; native D3D9 and installed DXVK GPU precision, tone-map,
UI-white, MSAA resolve, TAA history/copy and resize/reset fixtures; native-vs-HLSL
material comparisons with FP16 off and highlight fixtures with FP16 on; TAA shader
regression with BlingGFX first in the effective shader set. Actual gameplay visual
results, real HDR monitor presentation and performance remain for manual testing.
FP16 doubles bytes per colour pixel and adds two fullscreen copy/draw passes, so
there is a bandwidth/memory cost. No game FPS claim is made from the fixtures.
