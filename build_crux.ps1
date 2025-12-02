$ErrorActionPreference = "Stop"

function Find-VisualStudio {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    
    if (Test-Path $vswhere) {
        $path = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($path) {
            $vcvars = Join-Path $path "VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $vcvars) {
                return $vcvars
            }
        }
    }

    # Fallback to common paths
    $paths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($path in $paths) {
        if (Test-Path $path) {
            return $path
        }
    }
    return $null
}

Write-Host "[Setup] Looking for Visual Studio..." -ForegroundColor Cyan
$vcvars = Find-VisualStudio

if ($null -eq $vcvars) {
    Write-Error "Could not find Visual Studio installation."
}

Write-Host "[Setup] Found vcvars64.bat at: $vcvars" -ForegroundColor Green

# Create a temporary batch file to set env and run cl
# Added Winhttp.lib and other likely dependencies
$cmdContent = @"
call "$vcvars"
cl /EHsc /std:c++17 /DDEBUG source\CRUX_25.cc source\c2\comm.cc source\c2\crypto.cc source\c2\pastebin.cc source\footprinting\GetPidFromNtQuerySystemInformation.cc source\footprinting\footprinting.cc user32.lib kernel32.lib winhttp.lib Advapi32.lib Netapi32.lib /Fe:CRUX_25.exe
"@

Set-Content -Path "temp_build_crux.bat" -Value $cmdContent

Write-Host "[Build] Compiling CRUX_25.exe..." -ForegroundColor Cyan
& .\temp_build_crux.bat

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[Success] Compilation successful!" -ForegroundColor Green
} else {
    Write-Host "`n[Error] Compilation failed." -ForegroundColor Red
}

Remove-Item "temp_build_crux.bat" -ErrorAction SilentlyContinue
