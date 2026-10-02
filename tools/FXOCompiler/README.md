# SR2 FXOCompiler

Compiles HLSL into matching SR2 FXO files. D3DCompiler_47 is embedded.

Build with VS2022 and Windows SDK:

```powershell
msbuild tools/FXOCompiler/FXOCompiler.vcxproj /p:Configuration=Release /p:Platform=x64
tools/FXOCompiler/.out/FXOCompiler.exe build shaders/colour/build.ini
```
