@echo off
setlocal enabledelayedexpansion

REM Get the ProjectDir without quotes and with trailing backslash
set PROJECT_DIR=%~1
if "%PROJECT_DIR%"=="" set PROJECT_DIR=.
if not "%PROJECT_DIR:~-1%"=="\" set PROJECT_DIR=%PROJECT_DIR%\

REM Get the git commit hash
for /f %%i in ('git rev-parse HEAD') do set GIT_COMMIT_HASH=%%i

REM Method 1: PowerShell
powershell -command "$utc = (Get-Date).ToUniversalTime(); Write-Host $utc.ToString('yyyy-MM-dd HH:mm:ss UTC')" > "%TEMP%\utctime.txt"
set /p BUILD_TIME_UTC=<"%TEMP%\utctime.txt"
del "%TEMP%\utctime.txt"

REM Fallback if PowerShell method fails
if "!BUILD_TIME_UTC!"=="" (
    REM Get local time and convert to UTC using timezone offset
    for /f "tokens=2 delims==" %%a in ('wmic os get LocalDateTime /value') do set datetime=%%a
    for /f "tokens=2 delims==" %%a in ('wmic timezone get Bias /value') do set tzBias=%%a
    
    set /a Hour=1!datetime:~8,2! - 100
    set /a Min=1!datetime:~10,2! - 100
    
    REM Convert bias to hours and minutes
    set /a BiasHour=!tzBias! / 60
    set /a BiasMin=!tzBias! %% 60
    
    REM Adjust for timezone
    set /a hour_utc=!Hour! + !BiasHour!
    set /a min_utc=!Min! + !BiasMin!
    
    REM Handle minute overflow
    if !min_utc! geq 60 (
        set /a hour_utc+=1
        set /a min_utc-=60
    )
    if !min_utc! lss 0 (
        set /a hour_utc-=1
        set /a min_utc+=60
    )
    
    REM Handle hour overflow
    if !hour_utc! geq 24 (
        set /a hour_utc-=24
    )
    if !hour_utc! lss 0 (
        set /a hour_utc+=24
    )
    
    REM Format hours and minutes
    if !hour_utc! lss 10 set hour_utc=0!hour_utc!
    if !min_utc! lss 10 set min_utc=0!min_utc!
    
    set BUILD_TIME_UTC=!datetime:~0,4!-!datetime:~4,2!-!datetime:~6,2! !hour_utc!:!min_utc!:!datetime:~12,2! UTC
)

REM Create the output directory if it doesn't exist
if not exist "%PROJECT_DIR%Generated" mkdir "%PROJECT_DIR%Generated"

REM Create a temporary file with the replaced values
(
  echo #pragma once
  echo // Auto-generated file - Do not edit directly
  echo #include ^<string^>
  echo // Git commit hash
  echo inline const char* GIT_COMMIT_HASH = "!GIT_COMMIT_HASH!";
  echo // Build date and time in UTC
  echo inline const char* BUILD_TIME_UTC = "!BUILD_TIME_UTC!";
) > "%PROJECT_DIR%Generated\BuildInfo.h.tmp"

REM Only update the real file if different (avoids unnecessary rebuilds)
fc "%PROJECT_DIR%Generated\BuildInfo.h.tmp" "%PROJECT_DIR%Generated\BuildInfo.h" > nul 2>&1
if errorlevel 1 (
  move /y "%PROJECT_DIR%Generated\BuildInfo.h.tmp" "%PROJECT_DIR%Generated\BuildInfo.h"
) else (
  del "%PROJECT_DIR%Generated\BuildInfo.h.tmp"
)

exit 0