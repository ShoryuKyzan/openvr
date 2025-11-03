@echo off
setlocal

REM Resolve repository root relative to this script (two levels up)
pushd "%~dp0..\..\" >nul
set "REPO_ROOT=%CD%"
popd >nul

REM Get first 6 characters of git SHA
for /f %%i in ('git rev-parse --short^=6 HEAD') do set "gitsha=%%i"

REM Create zip file using PowerShell
set OUTPUT=%REPO_ROOT%\output\drivers\simplehmd_%gitsha%.zip
powershell -Command "Compress-Archive -Path '%REPO_ROOT%\output\drivers\simplehmd\*' -DestinationPath '%OUTPUT%' -Force"

echo Created release archive at: %OUTPUT%
