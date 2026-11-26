# Netzac project directives

Native **C++17 / Qt 6 WebEngine** recreation of classic Netscape Navigator 4.0 for Mac OS 7/8, styled with ZacOS 9's native Platinum look (`zacos9` Qt style, `ZacOS 9 System` / `ZacOS 9 Views` fonts, and `pl_bevel_button_paint` geometry) and localized across Linux (`dcgettext`) and Windows (`lrelease` `.qm` / `QTranslator`).

## Language, Stack & Build

- **Language & Standard**: Modern C++17 (`cpp_std=c++17`, `warning_level=2` in `meson.build`).
- **Frameworks**: Qt 6 (`Core`, `Gui`, `Network`, `Widgets`, `WebEngineCore`, `WebEngineWidgets`).
- **Build System**: Meson + Ninja (`./build_and_run.sh`, or `meson setup build && ninja -C build`).
- **Cross-Platform Targets**: Primary target is **ZacOS 9 / Debian Linux**; **Windows** is also supported (`#if defined(Q_OS_WIN)` guards for fonts, icons, desktop/registry helpers, and `.qm` translations). Keep `#if defined(Q_OS_WIN)` / `#if defined(Q_OS_LINUX)` boundaries intact.

## How Agents Should Work on Code

1. **Platinum Look & Widget Conventions**:
   - Inherit dialogs from `PlatinumDialog` and build buttons via `makeDialogButton()` (`dialogs.h`).
   - Use `zacosFont("ZacOS 9 System", ...)` / `zacosFont("ZacOS 9 Views", ...)` and existing Platinum widgets (`PlatinumNavButton`, `PlatinumLocationBar`, `PlatinumPopupButton`, `PlatinumProgressBar`, `PlatinumTabBar`, `NetzacThrobber`) in `widgets.h` / `widgets.cpp`.
   - Keep `QPainter` pixel-crisp (`Antialiasing` disabled for bevels/classic icons) and respect dark/light palettes via `platinumColors(palette())`.
2. **Qt & WebEngine Rules**:
   - Environment variables affecting Qt/Chromium (`QT_QPA_PLATFORM`, `QT_QUICK_BACKEND`, `QTWEBENGINE_CHROMIUM_FLAGS`) and `Qt::AA_ShareOpenGLContexts` **must** be configured in `main.cpp` before instantiating `QApplication`.
   - Preserve the Wayland mitigations in `main.cpp` (`WLR_RENDERER=pixman`, NVIDIA on Wayland → `xcb` XWayland fallback when `QT_QPA_PLATFORM` allows `xcb`, and the GPU-disable preference).
   - `QWebEngineProfile::downloadRequested` requires `accept()` or `cancel()` synchronously inside the signal handler; any modeless download UI (like `DownloadDialog`) must `accept()` (and `pause()` if waiting for user confirmation) immediately.
   - Keep UI operations non-blocking where navigation shouldn't freeze (e.g. modeless `DownloadDialog`).
3. **Verification**:
   - Always verify changes by compiling with `ninja -C build` (zero errors/warnings).
   - Always validate modified translation files with `msgfmt --check -o /dev/null po/*.po` and verify `.mo` generation in `ninja -C build`.

## Codebase & Feature Map

- `shell/netzac/main.cpp`: Application bootstrap, i18n initialization, Wayland/NVIDIA/pixman/GPU startup configuration, and `downloadRequested` → `DownloadDialog` wiring.
- `shell/netzac/netzacwindow.h` / `.cpp`: Main browser window, menus, collapsible Platinum toolbars, multi-tab management (drag reorder, detach/merge windows, IE4.5-left or Chrome-top tab bar), bookmarks/history integration, find bar, password autofill, and `about:*` built-in pages.
- `shell/netzac/webview.h` / `.cpp`: `NetzacWebView` & `NetzacWebPage`, Platinum CSS scrollbars, context menus, and popup handling.
- `shell/netzac/widgets.h` / `.cpp`: Custom Platinum UI controls (location bar with elided history dropdown, bevel buttons, throbber, viewport frame, progress bar, tab bar).
- `shell/netzac/dialogs.h` / `.cpp`: `PreferencesDialog` (Navigator, Calm Mode, Personal Toolbar, Quick Links, Cookies, Cache, Proxies, User Agent, Ad Blocker), `BookmarksDialog`, `AboutDialog`, `PlatinumAlertDialog`, and non-blocking IE-style `DownloadDialog` (globe-to-folder animation, temp/custom destination, auto-open, transfer stats & timers).
- `shell/netzac/preferences.h` / `.cpp`: Persistent `QSettings("zacos9", "netzac")` configuration, search engines, User-Agent presets (`chrome_linux`, `chrome_windows`, `firefox_linux`, `safari_macos`, `netzac_mac`, `custom`), GPU acceleration compatibility flag, and ZacOS Keychain password storage.
- `shell/netzac/calmmode.h` / `.cpp`: Calm Mode distraction stripping & `Readability.js` reader view with per-host exclusions.
- `shell/netzac/adblocker.h` / `.cpp`: EasyList/EasyPrivacy/URLHaus network filter engine and list downloader/cache.
- `shell/netzac/bookmarks.h` / `.cpp` & `history.h` / `.cpp`: Bookmark and browsing history persistence.
- `shell/netzac/platinum.h` / `.cpp`: `zacos9` theme application, font selection, and classic pixel icons.

## Localization & Translations (Mandatory)

- **Source Language**: Write all user-facing strings in C++ in **English** and wrap every one with `ztr("...")` or `ztrn("...", "...", n)` (`shell/netzac/i18n.h`). Do not concatenate translated fragments where a formatted string like `ztr("Size: %1").arg(...)` should be used.
- **Always Implement ALL Translations**: Whenever you add, modify, or remove any `ztr(...)` / `ztrn(...)` string in C++ (or desktop entry metadata in `share/applications/netzac.desktop`), you **must** update:
  1. The template: `po/zacos9-netzac.pot`
  2. **Every** language listed in `po/LINGUAS`:
     - `po/ar.po` (Arabic)
     - `po/es.po` (Spanish)
     - `po/fr_CA.po` (Canadian French)
     - `po/fr_FR.po` (French)
     - `po/it.po` (Italian)
     - `po/ja.po` (Japanese)
     - `po/pt.po` (Brazilian Portuguese)
     - `po/ru.po` (Russian)
- Never leave untranslated empty `msgstr ""` entries in any `.po` file, and always check for existing `msgid`s before appending to avoid duplicate-definition errors in `msgfmt`.
- Validate all catalogs after editing:
  ```sh
  for f in po/*.po; do msgfmt --check -o /dev/null "$f" || exit 1; done
  ```
