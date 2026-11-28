#Requires -Version 5.1
<#
.SYNOPSIS
    Build zacos9-netzac on Windows (MSVC 2022 + Qt 6 WebEngine) and produce a
    self-contained portable folder, ZIP archive, and/or Inno Setup installer.

.DESCRIPTION
    The Unix/ZacOS build is untouched: this script only drives Meson/Ninja with
    the MSVC toolchain, then runs windeployqt so the resulting folder runs on a
    machine without Qt installed. Optionally packages a portable ZIP (-Zip)
    and/or an Inno Setup installer (-Installer).

.EXAMPLE
    scripts\build-windows.ps1

.EXAMPLE
    scripts\build-windows.ps1 -QtDir C:\Qt\6.8.3\msvc2022_64 -Zip

.EXAMPLE
    scripts\build-windows.ps1 -Zip -ZipPath zacos9-netzac_1.0.0_windows-x64.zip -Installer -InstallerPath zacos9-netzac_1.0.0_windows-x64-setup.exe -Version 1.0.0
#>
[CmdletBinding()]
param(
    [string]$QtDir,
    [string]$BuildDir = 'build-windows',
    [string]$DistDir = 'build-windows\dist',
    [ValidateSet('release', 'debug', 'debugoptimized')]
    [string]$BuildType = 'release',
    [switch]$SkipDeploy,
    [switch]$Zip,
    [string]$ZipPath,
    [switch]$Installer,
    [string]$InstallerPath,
    [string]$Version,
    [string]$IsccPath
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

# Prefer the repository virtualenv when it exists (CI and local helper installs).
$venvBin = Join-Path $root '.venv\Scripts'
if ((Test-Path $venvBin) -and (($env:Path -split ';') -notcontains $venvBin)) {
    $env:Path = $venvBin + ';' + $env:Path
}

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
        if (-not $ZipPath) { $ZipPath = 'zacos9-netzac-windows-x64.zip' }
        if (-not [System.IO.Path]::IsPathRooted($ZipPath)) {
            $ZipPath = Join-Path $root $ZipPath
        }
        $zipDir = Split-Path -Parent $ZipPath
        if ($zipDir) { New-Item -ItemType Directory -Path $zipDir -Force | Out-Null }
        # Compress-Archive appends ".zip" when the destination does not have it,
        # so the temporary archive keeps a .zip extension.
        $tempZip = [System.IO.Path]::ChangeExtension($ZipPath, '.tmp.zip')
        Write-Step "Creating $ZipPath"
        if (Test-Path $tempZip) { Remove-Item $tempZip -Force }
        try {
            Compress-Archive -Path (Join-Path $DistDir '*') -DestinationPath $tempZip -Force -ErrorAction Stop
        } catch {
            throw ("could not create the zip (is the application still running and " +
                "locking a file in $DistDir?): " + $_.Exception.Message)
        }
        if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
        Move-Item $tempZip $ZipPath -Force
    }

    if ($Installer) {
        $distExe = Join-Path $DistDir 'zacos9-netzac.exe'
        if (-not (Test-Path $distExe)) {
            throw "Deployed executable not found at '$distExe'. Run without -SkipDeploy first."
        }

        $iscc = $IsccPath
        if (-not $iscc) {
            $isccCmd = Get-Command iscc -ErrorAction SilentlyContinue
            if ($isccCmd) { $iscc = $isccCmd.Source }
        }
        if (-not $iscc) {
            $candidates = @(
                (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
                (Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe')
            )
            $iscc = $candidates | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
        }
        if (-not $iscc -or -not (Test-Path $iscc)) {
            throw 'Inno Setup 6 (ISCC.exe) was not found. Install Inno Setup 6 or pass -IsccPath explicitly.'
        }

        $resolvedVersion = $Version
        if (-not $resolvedVersion) {
            $mesonBuild = Join-Path $root 'meson.build'
            if (Test-Path $mesonBuild) {
                $mesonText = Get-Content $mesonBuild -Raw
                if ($mesonText -match "version\s*:\s*'([^']+)'") {
                    $resolvedVersion = $matches[1]
                }
            }
        }
        if (-not $resolvedVersion) { $resolvedVersion = '1.0.0' }

        $numericParts = @()
        if ($resolvedVersion -match '^(\d+(?:\.\d+){0,3})') {
            $numericParts = @($matches[1].Split('.'))
        }
        while ($numericParts.Count -lt 4) { $numericParts += '0' }
        $numericVersion = ($numericParts[0..3]) -join '.'

        if (-not $InstallerPath) {
            $InstallerPath = "zacos9-netzac_${resolvedVersion}_windows-x64-setup.exe"
        }
        if (-not [System.IO.Path]::IsPathRooted($InstallerPath)) {
            $InstallerPath = Join-Path $root $InstallerPath
        }
        $installerDir = Split-Path -Parent $InstallerPath
        if ($installerDir) { New-Item -ItemType Directory -Path $installerDir -Force | Out-Null }
        $installerBase = [System.IO.Path]::GetFileNameWithoutExtension($InstallerPath)

        $resolvedDistDir = (Resolve-Path $DistDir).Path
        $issScript = Join-Path $root 'scripts\netzac.iss'
        if (-not (Test-Path $issScript)) { throw "Inno Setup script not found: $issScript" }

        Write-Step "Creating installer $InstallerPath (ISCC: $iscc)"
        & $iscc '/Qp' `
            "/DAppVersion=$resolvedVersion" `
            "/DAppVersionNumeric=$numericVersion" `
            "/DSourceDir=$resolvedDistDir" `
            "/O$installerDir" `
            "/F$installerBase" `
            $issScript
        if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed.' }
        if (-not (Test-Path $InstallerPath)) { throw "Expected installer '$InstallerPath' was not created." }

        $installerSizeMb = [math]::Round(((Get-Item $InstallerPath).Length / 1MB), 1)
        Write-Step "Installer ready: $InstallerPath ($installerSizeMb MB)"
    }

    Write-Host "Executable: $(Resolve-Path $exe)" -ForegroundColor Green
}
finally {
    Pop-Location
}

exit 0
