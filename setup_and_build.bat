@echo off
setlocal

echo [Search] Looking for Visual Studio installation...

set "VS_PATHS=^
C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat^
;C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat^
;C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat^
;C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat^
;C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat^
;C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat^
;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat^
;C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars64.bat^
;C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat"

set "FOUND_VCVARS="

for %%P in ("%VS_PATHS:;=" "%") do (
    if exist %%P (
        set "FOUND_VCVARS=%%~P"
        goto :Found
    )
)

:Found
if defined FOUND_VCVARS (
    echo [Setup] Found vcvars64.bat at: "%FOUND_VCVARS%"
    call "%FOUND_VCVARS%"
) else (
    echo [Error] Could not find Visual Studio installation.
    echo Please run this script from a Developer Command Prompt or ensure VS is installed in a standard location.
    exit /b 1
)

echo.
echo [Build] Compiling footprint_agent.cc...
echo [Build] Compiling footprint_agent.cc...
cl /EHsc source\footprint_agent.cc source\footprinting\footprinting.cc source\footprinting\GetPidFromNtQuerySystemInformation.cc source\footprinting\GetPidFromNtQueryFileInformation.cc user32.lib netapi32.lib version.lib /Fe:footprint_agent.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [Success] Compilation successful!
    echo [Run] Running footprint_agent.exe...
    echo ---------------------------------------------------
    footprint_agent.exe
    echo ---------------------------------------------------
) else (
    echo.
    echo [Error] Compilation failed.
)

endlocal
