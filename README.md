# The Official Repository for Saints Row 2 Juiced Patch
This is the entire project of SR2 Juiced Patch in it's entirety minus some SR2 Reloaded code. \
Juiced Patch was originally based on Monkey Patch by [Scanti](https://github.com/scanti2) \
This project was originally created using Visual Studio 2022 using C++23 and may only be buildable using this setup.

Some main patching areas are all included in Monkey Patch/General/General.cpp and Monkey Patch/MainHooks.cpp

## Build Environments
There are three different build environments for Juiced Patch.
- Debug (Full Juiced Patch)
- Debug - LITE (Juiced Patch LITE)
- RELOADED (Full Juiced Patch + Extra stuff for RELOADED)

(RELOADED Build Environment is still included in this source code but the code we have for SR2 Reloaded is scrapped out of
this repository for security purposes, So code will not build properly when using the RELOADED Build Environment.)

## How to build?
- Run the Juiced Patch.sln file using VS 2022 on a version that supports C++23
- Make sure the configuration is set to "Debug" or "Debug - LITE"
- Click "Build" then "Build Reloaded Patch" or press CTRL+B
- The exported Juiced Patch "DFEngine.dll" should output in a folder corresponding to the configuration name in your Juiced Patch project folder.

## Active Developers
- [Uzis](https://github.com/theuzis)
- [Tervel](https://github.com/Tervel1337)
- [Clippy](https://github.com/Clippy95)
- [Scanti](https://github.com/scanti2)
- [Caboose](https://github.com/CabooseSayzWTF)

## Special Thanks
- [Scanti](https://github.com/scanti2) - Extra help with Juiced Patch + Created Monkey Patch
- [Caboose](https://github.com/CabooseSayzWTF) - Model, Texture and Chunk fixes for Juiced Patch.
- [cursey](https://github.com/cursey) - [safetyhook](https://github.com/cursey/safetyhook)
- [ThirteenAG](https://github.com/ThirteenAG) - [Hooking.Patterns Patcher](https://github.com/ThirteenAG/Hooking.Patterns)
- jason098 - Slew Mode Fixes
- [Silent](https://github.com/CookiePLMonster) - Helpful information on FPS Issues
- Zedek the Plague Doctor - Error Handler + Patching Methods
- [CHC](https://github.com/chc) - Openspy


## Extra Information
Our loose model, texture, etc patches are included in the "Mods" folder in this repository.




