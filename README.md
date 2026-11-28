# Netzac Navigator 1.0 for ZacOS 9
<p align="center">
<img src="share/icons/hicolor/scalable/apps/netzac.svg" width="64">
</p>

<img src="https://g3pix.com.br/libs/software/netzac/6.png">

Native C++17 / Qt 6 WebEngine recreation of classic Navigator 4.0 for Mac OS 7/8,
styled with ZacOS 9's native Platinum look (`zacos9` Qt style, `ZacOS 9 System` /
`ZacOS 9 Views` fonts, and `pl_bevel_button_paint` geometry) and wired for GNU gettext
localization (`dcgettext`).

## Project Structure (ZacOS 9 Upstream Layout)

```
netzac/
├── meson.build
├── meson_options.txt
├── share/applications/zacos9-netzac.desktop
├── po/
│   ├── meson.build
│   ├── LINGUAS
│   ├── zacos9-netzac.pot
│   ├── ar.po
│   ├── de.po
│   ├── es.po
│   ├── fr_CA.po
│   ├── fr_FR.po
│   ├── it.po
│   ├── ja.po
│   ├── pt.po
│   ├── ru.po
│   └── zh_CN.po
└── shell/netzac/
    ├── meson.build
    ├── i18n.h
    ├── platinum.h / platinum.cpp
    ├── widgets.h / widgets.cpp
    ├── dialogs.h / dialogs.cpp
    ├── adblocker.h / adblocker.cpp
    ├── netzacwindow.h / netzacwindow.cpp
    └── main.cpp
```

## Tabs and the Netzac Ad Blocker

Tabs can be reordered by clicking and dragging them inside the tab bar. When multiple
tabs are open, dragging a tab outside the window detaches it into a new Netzac window
(the page keeps its history), and dragging a tab onto another Netzac window's tab bar
or toolbar merges it into that window.

The Netzac Ad Blocker (Preferences → Ad Blocker) blocks advertisement, tracking and
malicious requests using EasyList, EasyPrivacy and the Malicious URL (URLHaus)
domain list by default. Additional filter list URLs can be added; lists are cached
under `~/.cache/zacos9/netzac/adblock` and refreshed automatically once a day (or
immediately with **Update Filter Lists Now**). Host/anchor network rules are
enforced; cosmetic element-hiding rules are not applied.

## Build Dependencies (Debian Trixie / ZacOS 9)

```sh
sudo apt install meson ninja-build pkg-config gettext qt6-base-dev qt6-webengine-dev -y
```

## Building & Running Standalone

```sh
meson setup build
ninja -C build
./build/shell/netzac/zacos9-netzac
```

To test translations before installing system-wide:

```sh
ninja -C build zacos9-netzac-gmo
DESTDIR="$PWD/stage" ninja -C build install
ZACOS9_LOCALEDIR="$PWD/stage/usr/local/share/locale" ./build/shell/netzac/zacos9-netzac
```

## Windows Build & Packaging

Windows builds use Meson/Ninja with the MSVC 2022 toolchain and Qt 6 WebEngine;
`windeployqt` then produces a self-contained folder that runs without a Qt
installation or the VC++ Redistributable on the target machine. From that folder,
`scripts\build-windows.ps1` can create both a portable ZIP archive (`-Zip`) and
a Windows installer executable (`-Installer`, compiled with Inno Setup 6 via
`scripts\netzac.iss` using `LICENCE`).

Requirements:

- Visual Studio 2022 with the **Desktop development with C++** workload.
- Qt 6 for MSVC 2022 x64 with the WebEngine, WebChannel and Positioning modules:

  ```bat
  aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -O C:\Qt -m qtwebengine qtwebchannel qtpositioning
  ```

- Meson and Ninja (`python -m pip install meson ninja`).
- [Inno Setup 6](https://jrsoftware.org/isinfo.php) (optional, only required when passing `-Installer`).

```powershell
# Build, deploy the Qt runtime and create a portable ZIP
scripts\build-windows.ps1 -Zip

# Build both the portable ZIP and the Inno Setup installer
scripts\build-windows.ps1 -QtDir C:\Qt\6.8.3\msvc2022_64 -Version 1.0.0 -Zip -Installer

# Same, with explicit output filenames
scripts\build-windows.ps1 -QtDir C:\Qt\6.8.3\msvc2022_64 -Version 1.0.0 `
  -Zip -ZipPath zacos9-netzac_1.0.0_windows-x64.zip `
  -Installer -InstallerPath zacos9-netzac_1.0.0_windows-x64-setup.exe
```

The deployed folder is written to `build-windows\dist`. With `-Zip`, the archive
is written to `zacos9-netzac-windows-x64.zip` (or the `-ZipPath` value), and
with `-Installer`, the setup executable is written to
`zacos9-netzac_<version>_windows-x64-setup.exe` (or the `-InstallerPath` value).
Use `-SkipDeploy` to only compile the executable.

## Credits & Community

Netzac started as an initial idea by **Lucas Guimarães**. We welcome contributions,
bug reports, and pull requests from the **ZacOS 9** community at:
<https://github.com/guimaraeslucas/netzac>

Special thanks to **Adam Lawson (@chromasystemprofessional)** for creating the
incredible **ZacOS 9** project.

## Wayland, NVIDIA & Pixman Resize / Freeze Mitigation

- **Software compositor (`WLR_RENDERER=pixman`)**: Under `zacos9-session`, when `WLR_RENDERER=pixman` is active (software compositor on VMs or non-3D GPUs), Chromium's default GPU compositing inside `QWebEngineView` can stall on maximize/resize. `main.cpp` sets `Qt::AA_ShareOpenGLContexts` and, when `WLR_RENDERER=pixman` is detected, defaults `QT_QUICK_BACKEND=software` and `QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu --disable-gpu-compositing` (unless already set in the environment).
- **NVIDIA proprietary driver on Wayland (`wlroots` + `egl-wayland`)**: When embedding `QWebEngineView` inside `QMainWindow`, Qt promotes the entire client surface from `QWaylandShmBackingStore` to `QOpenGLCompositorBackingStore` / `QWaylandEglWindow` (`wl_egl_window`). On NVIDIA's `egl-wayland` + `wlroots` GL/EGL path, shrinking or restoring a maximized window can trigger a buffer stride/pitch desync that leaves the client area diagonally sheared/ghosted while the compositor-drawn title bar remains intact. When Wayland, an active XWayland `DISPLAY`, and the NVIDIA driver are detected—and `QT_QPA_PLATFORM` permits `xcb` (such as the `wayland;xcb` session default)—`main.cpp` automatically selects `QT_QPA_PLATFORM=xcb` (XWayland) so full GPU-accelerated web rendering stays enabled without resize shearing. Passing `-platform wayland` or setting `QT_QPA_PLATFORM=wayland` explicitly preserves native Wayland.
- **Incompatible VMs / Drivers**: You can toggle **Edit → Preferences → Navigator → Disable GPU hardware acceleration (compatibility mode, restart required)** to switch Qt Quick and Chromium to software rendering when GPU acceleration is incompatible. Keeping it unchecked provides the fastest web page rendering.

## Standalone & ZacOS 9 Extras Packaging

Netzac lives at <https://github.com/guimaraeslucas/netzac> and builds/runs
independently without requiring the core ZacOS 9 source tree, while remaining
compatible with ZacOS 9 Extras packaging (`package.conf` and `install_tag: 'netzac'`).

```sh
# Install for the current user (~/.local/bin, ~/.local/share/applications)
./scripts/install-user.sh

# Build a Debian package (build/packages/zacos9-extra-netzac_<version>_<arch>.deb)
./scripts/build-deb.sh 1.0.0
```

## Publishing a Release

The **Release Debian package** GitHub Actions workflow builds
`zacos9-extra-netzac_<version>_amd64.deb` and `zacos9-extra-netzac_<version>_arm64.deb`
on Debian Trixie and attaches them, along with SHA-256 checksums, to the matching GitHub
Release.

The **Release Windows package** workflow builds both
`zacos9-netzac_<version>_windows-x64.zip` (portable archive) and
`zacos9-netzac_<version>_windows-x64-setup.exe` (Inno Setup installer), along
with their SHA-256 checksums, on `windows-latest` with MSVC 2022 and Qt 6
WebEngine, and attaches them to the same GitHub Release.

Publish a release by pushing a version tag:

```sh
git tag v1.0.0
git push origin v1.0.0
```

Both workflows also run when a GitHub Release is published, so creating the release from
the GitHub UI attaches the packages too. To build the first release, or to rebuild an
existing tag, open **Actions → Release Debian package** (or **Release Windows package**)
**→ Run workflow** and enter the version without the leading `v` (for example `1.0.0`).

## Screenshots
<img src="https://g3pix.com.br/libs/software/netzac/3.png" width="200"> <img src="https://g3pix.com.br/libs/software/netzac/4.png" width="200"> <img src="https://g3pix.com.br/libs/software/netzac/7.png" width="200">
