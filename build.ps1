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

    # Fallback to common paths if vswhere fails or isn't found
    $paths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
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
    Write-Error "Could not find Visual Studio installation. Please ensure C++ workload is installed."
}

Write-Host "[Setup] Found vcvars64.bat at: $vcvars" -ForegroundColor Green

# Create a temporary batch file to set env and run cl
$cmdContent = @"
call "$vcvars"
cl /EHsc source\footprint_agent.cc source\footprinting\footprinting.cc source\footprinting\GetPidFromNtQuerySystemInformation.cc source\footprinting\GetPidFromNtQueryFileInformation.cc user32.lib netapi32.lib version.lib /Fe:footprint_agent.exe
"@

Set-Content -Path "temp_build.bat" -Value $cmdContent

Write-Host "[Build] Compiling footprint_agent.exe..." -ForegroundColor Cyan
& .\temp_build.bat

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[Success] Compilation successful!" -ForegroundColor Green
    Write-Host "[Run] Running footprint_agent.exe...`n" -ForegroundColor Cyan
    & .\footprint_agent.exe
} else {
    Write-Host "`n[Error] Compilation failed." -ForegroundColor Red
}

Remove-Item "temp_build.bat" -ErrorAction SilentlyContinue
