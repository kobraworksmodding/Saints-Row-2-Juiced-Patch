# Saints Row 2 Juiced Patch

Juiced Patch is a collaboration of fixes and enhancements for Saints Row 2 created by Uzis, Tervel, Clippy95 and Scanti
using an inject-able DLL file running using the Monkey Patch framework backend by Scanti,
Juiced Patch provides these fixes and enhancements to the Saints Row 2 PC port, only supporting the latest uncensored update of the game.

[Featured fixed that Juiced provides](FEATURED_FIXES.md)

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

## Contributing
This [issues list](https://github.com/kobraworksmodding/SR2IssuesList/issues) is a good lead on SR2-specific issues to attempt to fix.
* Contributions are very much welcome for the game and for any Juiced bugs, and we are willing to help with any issues/contributions if you open an issue/PR.
* Netcode is out-of-scope, if there's a feature you want to implement that uses networking contact us.
* This repository's issues is not meant for technical support.

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




