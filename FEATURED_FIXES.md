# Featured fixes

The following are just featued fixes and doesn't encompass all of Juiced's fixes/additions.

Fixes marked with ⚙️ can be configured/toggled via the INI file / BlingMenu

## Critical fixes

* ⚙️ FPS boost by at least 2x[^1] via SleepHack (3 options via **SleepHack**)
* ⚙️ Fix Havok Frametime which casued objects,cutscenes,camera math and more to be incorrect/desync. 
* ⚙️ Correct Gamma to be more inline with the Xbox 360 version.
* ⚙️ Use XACT 2.3 which in turn fixes several audio playback issues, aside from 3D panning
* Automatically LAA patch SR2's exe, (including Steam)
* Fix semi-common texture crash that was caused in "add_to_entry"
* Fix water being black in the distance.
* Allow users to select current resolution from in-game options.
[^1]: Sleephack effects are greatly enhanced with the usage of [DXVK](https://github.com/doitsujin/dxvk).
## Other fixes
* Motion blur math scales with framerate.
* Fix sunflare disappearing upon a save.
* Fix cutscenes resetting shadow setting.
* Fix ghost mouse scroll inputs when tabbing in and out
* Prevent scroll wheel usage when player is idling.
* Add 8x MSAA option
* Patch Gamespy with Openspy.
* ⚙️ Fix mouse input in tagging minigame.
* ⚙️ Fix mouse input in wardrobes.
* Fix Ultrawide FOV
* ⚙️ Fix incorrect GUID assignment for audio device.

## Enhancements
* Allow loading of loose files (VPP content) with folders to load from taken from loose.txt (default mods folder)
* ⚙️ Restore Slew Mode, toggleable via F4 or BlingMenu
* ⚙️ Lua executor with some custom commands.
* ⚙️ <kbd>F</kbd> Now serve several useful and helpful functions such as HUD toggle, Slew, FOV & more.
* ⚙️ VanillaFXPlus - Overhauls the lighting, keeping the SR2 feel while removing the orangey/yellow screen filter. (off by default)
* ⚙️ ShadowMapFiltering - Applies a smooth filtering shader to tree shadow maps (off by default)
* ⚙️ UHQScreenEffects - Raises resolution of Depth-of-field, skydive blur, reflections and bloom
* ⚙️ BetterAmbientOcclusion - Smoothens the Ambient occlusion
* ⚙️ Skip Intro (off by default)
* ⚙️ DisableCheatFlag (off by default)
* ⚙️ LoadLastSave (off by default)
* ⚙️ BetterDriveByCam - remove locking of the horizontal axis when free-cam is toggled in vehicle's camera.
* ⚙️ BetterHandbrakeCam - disables handbrake cam locking (off by default)
* ⚙️ AllowToggleCrouchWhileWalk 
* ⚙️ SR1Reloading - Allows you to reload while sprinting.
* ⚙️ SR1Quickswitch - Restores the quick switching technique from SR1 (off by default)
* ⚙️ TauntCancelling
* ⚙️ UseWeaponAfterEmpty - Keep your empty weapon equipped after you've run out of ammo for that weapon
* ⚙️ RemoveVignette (off by default)
* ⚙️ FasterLoadingScreens