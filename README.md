# Netzac Navigator for ZacOS 9

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

## Wayland / Pixman Maximize Freeze Mitigation

Under `zacos9-session`, when `WLR_RENDERER=pixman` is active (software compositor on VMs
or non-3D GPUs), Chromium's default GPU compositing inside `QWebEngineView` can stall on
maximize/resize. `main.cpp` sets `Qt::AA_ShareOpenGLContexts` and, when
`WLR_RENDERER=pixman` is detected, defaults `QT_QUICK_BACKEND=software` and
`QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu --disable-gpu-compositing` (unless already set
in the environment).

## Merging into the ZacOS 9 Repository

1. Copy `shell/netzac/` into `shell/netzac/` in the `zacos9` source tree and add
   `subdir('shell/netzac')` to the root `meson.build` (or pass `-Dtextdomain=zacos9` so
   strings resolve against the unified `zacos9` domain).
2. Copy `share/applications/zacos9-netzac.desktop` to `share/applications/`.
3. Merge the messages from `po/zacos9-netzac.pot` and `po/pt.po` into ZacOS 9's `po/`
   catalog (using `ztr:1` and `ztrn:1,2` xgettext keywords).
