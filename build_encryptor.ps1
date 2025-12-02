# Script to build and run the encryptor tool

# 1. Find Visual Studio
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Host "[Error] vswhere.exe not found."
    exit 1
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    Write-Host "[Error] Visual Studio with C++ tools not found."
    exit 1
}

Write-Host "[Setup] Found Visual Studio at: $vsPath"

# 2. Find vcvars64.bat
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    Write-Host "[Error] vcvars64.bat not found at: $vcvars"
    exit 1
}

# 3. Compile encryptor.cc
Write-Host "[Build] Compiling encryptor..."
$buildCmd = @"
call "$vcvars"
cl /EHsc tools\encryptor.cc /Fe:tools\encryptor.exe
"@

Set-Content -Path "temp_build_encryptor.bat" -Value $buildCmd
Start-Process -FilePath "cmd.exe" -ArgumentList "/c temp_build_encryptor.bat" -Wait -NoNewWindow
Remove-Item "temp_build_encryptor.bat"

if (-not (Test-Path "tools\encryptor.exe")) {
    Write-Host "[Error] Compilation failed."
    exit 1
}

# 4. Run encryptor and update shcode.h
Write-Host "[Run] Generating encrypted shellcode..."
& "tools\encryptor.exe" > "source\shcode.h"

if ($LASTEXITCODE -eq 0) {
    Write-Host "[Success] source\shcode.h updated with encrypted payload."
} else {
    Write-Host "[Error] Failed to generate shellcode."
}
