@echo off
REM This assumes you are in the VS2019 Developer Command Prompt

REM Resolve repository root relative to this script (two levels up)
pushd "%~dp0..\..\" >nul
set "REPO_ROOT=%CD%"
popd >nul

REM Build command for Win32 Release
msbuild "%REPO_ROOT%\vs-openvr_samples.sln" /t:simplehmd /p:Configuration=Release /p:Platform=x86
if %ERRORLEVEL% NEQ 0 (
    echo "msbuild (x86) failed with error %ERRORLEVEL%."
    exit /b %ERRORLEVEL%
)

msbuild "%REPO_ROOT%\vs-openvr_samples.sln" /t:simplehmd /p:Configuration=Release /p:Platform=x64
if %ERRORLEVEL% NEQ 0 (
    echo "msbuild (x64) failed with error %ERRORLEVEL%."
    exit /b %ERRORLEVEL%
)

REM Copy driver files to SteamVR drivers directory
xcopy /Y /E /I "%REPO_ROOT%\output\drivers\simplehmd" "%ProgramFiles(x86)%\Steam\steamapps\common\SteamVR\drivers\simplehmd"

REM Launch SteamVR
start steam://rungameid/250820