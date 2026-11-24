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
│   └── pt.po
└── shell/netzac/
    ├── meson.build
    ├── i18n.h
    ├── platinum.h / platinum.cpp
    ├── widgets.h / widgets.cpp
    ├── dialogs.h / dialogs.cpp
    ├── netzacwindow.h / netzacwindow.cpp
    └── main.cpp
```

## Build Dependencies (Debian Trixie / ZacOS 9)

```sh
sudo apt install meson ninja-build pkg-config gettext qt6-base-dev qt6-webengine-dev
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

## Credits & Community

Netzac started as an initial idea by **Lucas Guimarães**. We welcome contributions,
bug reports, and pull requests from the **ZacOS 9** community at:
<https://github.com/guimaraeslucas/netzac>

Special thanks to **Adam Lawson (`@chromasystemprofessional`)** for creating the
incredible **ZacOS 9** project.

## Wayland / Pixman Maximize Freeze Mitigation

Under `zacos9-session`, when `WLR_RENDERER=pixman` is active (software compositor on VMs
or non-3D GPUs), Chromium's default GPU compositing inside `QWebEngineView` can stall on
maximize/resize. `main.cpp` sets `Qt::AA_ShareOpenGLContexts` and, when
`WLR_RENDERER=pixman` is detected, defaults `QT_QUICK_BACKEND=software` and
`QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu --disable-gpu-compositing` (unless already set
in the environment).

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
`zacos9-extra-netzac_<version>_<arch>.deb` on Debian Trixie and attaches it, along with
a SHA-256 checksum, to the matching GitHub Release.

Publish a release by pushing a version tag:

```sh
git tag v1.0.0
git push origin v1.0.0
```

The workflow also runs when a GitHub Release is published, so creating the release from
the GitHub UI attaches the package too. To build the first release, or to rebuild an
existing tag, open **Actions → Release Debian package → Run workflow** and enter the
version without the leading `v` (for example `1.0.0`).

## Screenshots
<img src="https://g3pix.com.br/libs/software/netzac/3.png" width="200"> <img src="https://g3pix.com.br/libs/software/netzac/4.png" width="200"> <img src="https://g3pix.com.br/libs/software/netzac/7.png" width="200">