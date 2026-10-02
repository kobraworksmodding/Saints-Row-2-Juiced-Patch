# Juiced colour shaders

`build.ini` is the native FXOCompiler manifest. `materials/` contains editable HLSL
for the colour-clipping portions of the loose shaders. `original/` holds immutable
matching FXO templates with the game's packed bindings and all shader variants.
The compiler replaces only the listed pixel programs and preserves the remaining
container bytes. The same manifest generates the embedded colour, distortion and
existing TAA headers. See [FP16 colour](../../docs/FP16_COLOUR.md) and the
[native compiler](../../tools/FXOCompiler/README.md) for build commands.

All shader registers keep the template's bindings. `c223.x` is reserved for the
Juiced renderer: zero retains native RGB clamps, one preserves positive highlight
headroom. Scalar saturation for coverage, fog, Fresnel and normal math is retained.
