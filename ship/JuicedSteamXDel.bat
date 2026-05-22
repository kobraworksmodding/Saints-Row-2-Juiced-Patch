@echo off
cd /d "%~dp0"

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo This script requires administrator privileges.
    echo Attempting to relaunch as administrator...
    powershell -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

if exist "SR2_pc.exe" (
    echo Backing up original SR2_pc.exe...
    attrib -r -s -h "SR2_pc.exe" >nul 2>&1
    ren "SR2_pc.exe" "sr2_pc_unpatched.exe"
)

if not exist "sr2_pc_unpatched.exe" (
    echo ERROR: sr2_pc_unpatched.exe not found.
    echo Please restore the original SR2_pc.exe and try again.
    pause
    exit /b
)

if not exist "patches\xdelta3.exe" (
    echo ERROR: xdelta3.exe not found.
    pause
    exit /b
)

if not exist "patches\JuicedSteamXDel.vcdiff" (
    echo ERROR: Patch file not found.
    pause
    exit /b
)


taskkill /f /im SR2_pc.exe >nul 2>&1
attrib -r -s -h "SR2_pc.exe" >nul 2>&1
del /f /q "SR2_pc.exe" >nul 2>&1

:: Apply patch
echo Patching and Fixing SR2...
"patches\xdelta3.exe" -d -s "sr2_pc_unpatched.exe" "patches\JuicedSteamXDel.vcdiff" "SR2_pc.exe"
if errorlevel 1 (
    echo ERROR: Patch failed. The input file may be invalid or already patched.
    pause
    exit /b
)

echo Patch applied successfully!
start SR2_pc.exe
