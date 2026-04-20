HOW TO INSTALL:

Open your Saints Row 2 Game Directory

Drag:
DFEngine.dll, 
mods (folder),
patches (folder), 
loose.txt, 
discord_game_sdk.dll, 
JuicedSteamXDel.bat, 
Juiced (folder), 
juiced.ini into the directory

Run game through "sr2_pc.exe". ( I would 10000% recommend to take a look at the juiced.ini file before you launch the game as it has key options in there that may or may not help your gameplay, fps, etc. )


Notes:

Teleport to Waypoint and LUA Console Executor both enable cheat flag if used.
FetchKWorksAd.exe is a completely optional binary file, you do not need to install it however you can run it before you launch the game for the first time to install community made billboard images.
VanillaFXPlus Replaces HDR Option. If you want to use original visuals for the game disable VanillaFXPlus in juiced.ini.
Borderless windowed replaces Windowed mode. To enabled borderless you just pick the windowed option in game.
AltTabFPS option simply removes the fps cap when alt tabbed.
UncapFPS uses more cpu usage the higher your fps goes.
GOG Executable is not needed but is HIGHLY recommended as its more stable and the patch was built using a GOG exe.
coopPausePatch can potentially cause issues in co-op.
Thanks to Scanti the Audio Playback has been fixed! 



--Custom Binds--
!!!TOGGLE AddBindToggles TO 1 IN JUICED.INI!!!
F1 Enables Ugly Mode
F2 Toggles HUD
F3 Toggles FPS Counter
F4 Enables Slew Mode
F5 Prints Player Coordinates to Debug Console
F6 Freezes Game Time (Doesn't work in Multiplayer)
F7 Teleports to Waypoint (Doesn't work in Multiplayer, enables cheat flag.)
F8 Lowers FOV
F9 Highers FOV
F11 Quick Save
Tab (Pause in cutscenes)

-- Slew Binds --
1 Cam Roll (In Slew)
3 Cam Roll (In Slew)
Middle Mouse Click - Toggle Depth-of-Field
Middle Mouse Scroll - FOV
Mouse side Buttons + Scroll - Adjust (Top mouse side button = Strength, Bottom mouse side button = Distance)


HELP:

1 : "My game is stuck on "Please Wait..." while trying to join a CO-OP partner or an Online Match."
    The PC port for this game is very finnicky in many aspects. Online is one. If you have more than 1 network adapter enabled on your computer
	that can totally ruin your chances of joining a lobby on SR2.
	Heres how you can try to fix that:
	
	( BOTH PLAYERS NEED MAKE SURE TO DO THIS ) 
	
	1. Close your game
	2. Hit your windows key or use cortana to search for "View network connections" on your PC and open it.
	3. You should see a screen containing all of the network connections/drivers your computer can access.
	4. Disable every single one besides the one that actually connects you to the internet.
	5. Reboot game, host or join a game and you should hopefully be golden.
	
	If none of that works, as a host you might need to Port Forward the port 4200 as a last resort, but in most cases this isn't even needed.
	
	We are currently also investigating this issue and looking at ways for it to be fixed if it can even be. We will notify on Discord if its been fixed.

2 : I dont have audio and/or my game is crashing when the legal screens pop-up.
    Turn off UseFixedXACT in the reloaded.ini config.
	On some systems UseFixedXACT may not work, A big example being Linux with Proton 
	
	(There have been reports that this setting doesn't work well on linux.)
	
3 : I crash when making a new save game or loading a save game.
    Turn ForceDisableVibration to 1 in the reloaded.ini config. You can also try turning DisableXInput to 1.
	
4 : Help my controller doesn't work when launching through steam or even at all
    Turn off steam input, if you still have issues try using an X-Input mapper like DS4-Windows, x360ce or xPadder
	
5 : Booting Juiced Patch for the first time and getting a write/read issue error?
    Start Saints Row 2 with Administrator Privileges.
