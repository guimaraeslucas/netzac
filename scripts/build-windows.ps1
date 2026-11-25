#Requires -Version 5.1
<#
.SYNOPSIS
    Build zacos9-netzac on Windows (MSVC 2022 + Qt 6 WebEngine) and produce a
    self-contained portable folder.

.DESCRIPTION
    The Unix/ZacOS build is untouched: this script only drives Meson/Ninja with
    the MSVC toolchain, then runs windeployqt so the resulting folder runs on a
    machine without Qt installed.

.EXAMPLE
    scripts\build-windows.ps1

.EXAMPLE
    scripts\build-windows.ps1 -QtDir C:\Qt\6.8.3\msvc2022_64 -Zip
#>
[CmdletBinding()]
param(
    [string]$QtDir,
    [string]$BuildDir = 'build-windows',
    [string]$DistDir = 'build-windows\dist',
    [ValidateSet('release', 'debug', 'debugoptimized')]
    [string]$BuildType = 'release',
    [switch]$SkipDeploy,
    [switch]$Zip
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

function Write-Step([string]$Message) {
    Write-Host "==> $Message" -ForegroundColor Cyan
}

try {
    # ------------------------------------------------------------------ Qt 6
    if (-not $QtDir) {
        $candidates = @()
        if ($env:QTDIR) { $candidates += $env:QTDIR }
        $candidates += Get-ChildItem 'C:\Qt' -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object {
                Get-ChildItem $_.FullName -Directory -ErrorAction SilentlyContinue |
                    Where-Object { $_.Name -like 'msvc*_64' } |
                    Select-Object -ExpandProperty FullName
            }
        $QtDir = $candidates | Where-Object { Test-Path (Join-Path $_ 'bin\qmake.exe') } |
            Select-Object -First 1
    }
    if (-not $QtDir -or -not (Test-Path (Join-Path $QtDir 'bin\qmake.exe'))) {
        throw 'Qt (MSVC 64-bit) was not found. Install it with: ' +
            'aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -O C:\Qt ' +
            '-m qtwebengine qtwebchannel qtpositioning ' +
            'or pass -QtDir explicitly.'
    }
    if (-not $env:QTDIR) { $env:QTDIR = $QtDir }
    $qtBin = Join-Path $QtDir 'bin'
    $env:Path = $qtBin + ';' + $env:Path
    Write-Step "Qt: $QtDir"

    # ----------------------------------------------------------------- MSVC
    $vsPath = $null
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (-not (Test-Path $vswhere)) {
            throw 'vswhere.exe not found. Install Visual Studio 2022 with the "Desktop development with C++" workload.'
        }
        $vsPath = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if (-not $vsPath) {
            throw 'No MSVC x64 toolset found. Install the "Desktop development with C++" workload.'
        }
        $vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
        if (-not (Test-Path $vcvars)) { throw "vcvars64.bat not found at $vcvars" }
        cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
            if ($_ -match '^([^=]+)=(.*)$') {
                [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
            }
        }
        if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
            throw 'MSVC environment could not be initialised.'
        }
    }
    Write-Step "MSVC: $((Get-Command cl.exe).Source)"

    if (-not $vsPath) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (Test-Path $vswhere) {
            $vsPath = & $vswhere -latest -products * `
                -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
                -property installationPath
        }
    }

    # ---------------------------------------------------------------- Meson
    $meson = (Get-Command meson -ErrorAction SilentlyContinue).Source
    if (-not $meson) {
        $venvMeson = Join-Path $root '.venv\Scripts\meson.exe'
        if (Test-Path $venvMeson) {
            $meson = $venvMeson
        } else {
            throw 'meson not found. Install it with: python -m pip install meson ninja'
        }
    }
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
        throw 'ninja not found. Install it with: python -m pip install ninja'
    }

    if (-not (Test-Path (Join-Path $BuildDir 'build.ninja'))) {
        Write-Step "Configuring ($BuildType)"
        & $meson setup $BuildDir "--buildtype=$BuildType"
        if ($LASTEXITCODE -ne 0) { throw 'meson setup failed.' }
    }

    Write-Step 'Building'
    & $meson compile -C $BuildDir
    if ($LASTEXITCODE -ne 0) { throw 'build failed.' }

    $exe = Join-Path $BuildDir 'shell\netzac\zacos9-netzac.exe'
    if (-not (Test-Path $exe)) { throw "executable not found: $exe" }

    # --------------------------------------------------------------- Deploy
    if (-not $SkipDeploy) {
        Write-Step "Deploying Qt runtime to $DistDir"
        if (Test-Path $DistDir) { Remove-Item $DistDir -Recurse -Force }
        New-Item -ItemType Directory -Path $DistDir -Force | Out-Null
        Copy-Item $exe $DistDir

        $windeployqt = Join-Path $qtBin 'windeployqt.exe'
        $deployArgs = @(
            if ($BuildType -eq 'release') { '--release' } else { '--debug' }
            '--no-translations'
            '--webenginewidgets'
            '--dir', $DistDir
            (Join-Path $DistDir 'zacos9-netzac.exe')
        )
        & $windeployqt @deployArgs
        if ($LASTEXITCODE -ne 0) { throw 'windeployqt failed.' }

        # VC++ runtime, app-local (no vc_redist installer needed on the target)
        $crtCopied = $false
        if ($vsPath) {
            $crtRoot = Join-Path $vsPath 'VC\Redist\MSVC'
            Get-ChildItem $crtRoot -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                ForEach-Object {
                    Get-ChildItem (Join-Path $_.FullName 'x64') -Directory -ErrorAction SilentlyContinue |
                        Where-Object { $_.Name -like 'Microsoft.VC*.CRT' }
                } |
                Select-Object -First 1 |
                ForEach-Object {
                    Get-ChildItem $_.FullName -Filter '*.dll' |
                        Copy-Item -Destination $DistDir -Force
                    $crtCopied = $true
                }
        }
        Remove-Item (Join-Path $DistDir 'vc_redist.x64.exe') -ErrorAction SilentlyContinue
        if (-not $crtCopied) {
            Write-Warning 'VC++ runtime DLLs were not copied; the target needs the VC++ Redistributable installed.'
        }

        # Application translations (.qm produced by lrelease from po/*.po)
        $translations = Join-Path $DistDir 'translations'
        New-Item -ItemType Directory -Path $translations -Force | Out-Null
        Get-ChildItem (Join-Path $BuildDir 'shell\netzac') -Filter '*.qm' -ErrorAction SilentlyContinue |
            Copy-Item -Destination $translations -Force

        # Application icon at runtime (no icon theme on Windows)
        foreach ($icon in @('share\icons\netzac.ico', 'share\icons\hicolor\scalable\apps\netzac.svg')) {
            if (Test-Path $icon) { Copy-Item $icon $DistDir }
        }

        $sizeMb = [math]::Round(((Get-ChildItem $DistDir -Recurse -File |
            Measure-Object Length -Sum).Sum / 1MB), 1)
        Write-Step "Portable folder ready: $DistDir ($sizeMb MB)"
    }

    if ($Zip) {
        $zipPath = Join-Path $root 'zacos9-netzac-windows-x64.zip'
        $tempZip = Join-Path $root 'zacos9-netzac-windows-x64.tmp.zip'
        Write-Step "Creating $zipPath"
        if (Test-Path $tempZip) { Remove-Item $tempZip -Force }
        try {
            Compress-Archive -Path (Join-Path $DistDir '*') -DestinationPath $tempZip -Force -ErrorAction Stop
        } catch {
            throw ("could not create the zip (is the application still running and " +
                "locking a file in $DistDir?): " + $_.Exception.Message)
        }
        if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
        Move-Item $tempZip $zipPath -Force
    }

    Write-Host "Executable: $(Resolve-Path $exe)" -ForegroundColor Green
}
finally {
    Pop-Location
}

exit 0
