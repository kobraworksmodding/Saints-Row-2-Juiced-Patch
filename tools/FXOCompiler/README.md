# SR2 FXOCompiler

Prebuilt EXE with D3DCompiler_47 embedded. Actions runs it directly:

```powershell
tools/FXOCompiler/FXOCompiler.exe build shaders/colour/build.ini
```

Rebuild only after compiler changes (VS2022 + Windows SDK):
`msbuild tools/FXOCompiler/FXOCompiler.vcxproj /p:Configuration=Release /p:Platform=x64`
Then replace `FXOCompiler.exe` with `.out/FXOCompiler.exe`.
