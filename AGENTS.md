# Netzac project directives

Native **C++17 / Qt 6 WebEngine** recreation of classic Netscape Navigator 4.0 for Mac OS 7/8, styled with ZacOS 9's native Platinum look (`zacos9` Qt style, `ZacOS 9 System` / `ZacOS 9 Views` fonts, and `pl_bevel_button_paint` geometry) and localized across Linux (`dcgettext`) and Windows (`lrelease` `.qm` / `QTranslator`).

## Language, Stack & Build

- **Language & Standard**: Modern C++17 (`cpp_std=c++17`, `warning_level=2` in `meson.build`).
- **Frameworks**: Qt 6 (`Core`, `Gui`, `Network`, `Widgets`, `PrintSupport`, `WebEngineCore`, `WebEngineWidgets`).
- **Build System**: Meson + Ninja (`./build_and_run.sh`, or `meson setup build && ninja -C build`).
- **Cross-Platform Targets**: Primary target is **ZacOS 9 / Debian Linux**; **Windows** is also supported (`#if defined(Q_OS_WIN)` guards for fonts, icons, desktop/registry helpers, and `.qm` translations). Keep `#if defined(Q_OS_WIN)` / `#if defined(Q_OS_LINUX)` boundaries intact.

## How Agents Should Work on Code

1. **Platinum Look & Widget Conventions**:
   - Inherit dialogs from `PlatinumDialog` and build buttons via `makeDialogButton()` (`dialogs.h`).
   - Use `zacosFont("ZacOS 9 System", ...)` / `zacosFont("ZacOS 9 Views", ...)` and existing Platinum widgets (`PlatinumNavButton`, `PlatinumLocationBar`, `PlatinumPopupButton`, `PlatinumProgressBar`, `PlatinumTabBar`, `NetzacThrobber`) in `widgets.h` / `widgets.cpp`.
   - Use `PlatinumPopupButton` instead of raw `QComboBox` for dropdown selectors in Preferences and dialogs (except font pickers that need scrollable family lists) so every choice control matches the ZacOS 9 Platinum popup button appearance.
   - Keep `QPainter` pixel-crisp (`Antialiasing` disabled for bevels/classic icons) and respect dark/light palettes via `platinumColors(palette())`.

## Iconography & Pixel-Art Style Guide

All built-in icons in `shell/netzac/platinum.cpp` (`makeNavIcon`, `makeMiniIcon`, `makeAlertIcon`) follow a cohesive **ZacOS 9 Platinum + Netscape Navigator 4.0 for Mac OS 7/8** pixel-art aesthetic:

- **Visual Identity (Original Classic Tribute)**: Evoke the charm, clarity, and tactile pixel craft of classic Macintosh System 1.0–8.x iconography (e.g., Susan Kare's expressive human/object glyphs) and Netscape Navigator 4.0's lavender/plum toolbar art — **never** copy Apple logos, trademarks, or exact Macintosh bitmaps verbatim, and prefer classic Mac OS / Netscape metaphors (vertical dog-eared documents, beveled Platinum padlocks, stylized human/object glyphs) over Windows/IE tropes (such as globe-behind-diploma icons). Every icon must be an original composition that feels native to ZacOS 9 Platinum. 
- **Signature Color Palette**:
  - **Primary Purple Ramp (Navigator Accent)**: Base `#685096`, Light `#8b72ba`, Mid-Light `#a98fd6`, Deep Shadow `#3b2a5e`.
  - **Soft Pink Highlights**: Edge highlight `#f4c6e7`, Soft fill/tint `#ffe3f5`.
  - **Plum / Berry Accent**: `#8b3a82` (ribbons, secondary trims).
  - **Crisp Structural Neutrals**: Outline `#000000`, Paper/Face `#ffffff`, Light gray `#eeeeee`, Mid gray `#999999`.
  - **Semantic Accents (use sparingly for universally recognized status cues)**: Gold/Amber `#f2c230` (dark `#996600`, light `#fff4aa`, warning `#ffcc00`) for locks, seals, and warning triangles; Stop Red `#cc2222` (highlight `#ff6655`) for Stop/critical cues.
- **Technical Pixel-Art Rules**:
  - **No Antialiasing**: Always set `p.setRenderHint(QPainter::Antialiasing, false);` and draw on integer coordinates.
  - **Canonical Grids**: Design on `20×20` (`makeNavIcon`), `14×14` (`makeMiniIcon`), `32×32` (`makeAlertIcon` `"info"`, `"question"`, `"warning"`), or `48×48` (`makeAlertIcon` `"certificate"`), scaling via `QPainter::scale()` only if a non-default size is requested.
  - **1px Black Silhouettes & Top-Left Lighting**: Frame major shapes with crisp 1px `#000000` outlines, add 1px top-left inner highlights (`#ffffff`, `#ffe3f5`, `#f4c6e7`, or `#8b72ba`), and anchor bottom-right edges with 1px shadow strokes (`#3b2a5e`) so icons sit naturally on Platinum bevels.
  - **Disabled State**: Toolbar icons automatically derive disabled states via `makeDisabledPixmap()`, so maintain strong silhouette contrast between foreground fills and outlines.
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
- `shell/netzac/dialogs.h` / `.cpp`: `PreferencesDialog` (Navigator, Calm Mode, Personal Toolbar, Quick Links, Cookies, Permissions, Cache, Proxies, User Agent, Ad Blocker), `PermissionPromptDialog` (Never/Just Now/Always), `BookmarksDialog`, `AboutDialog`, `PlatinumAlertDialog`, `CertificateInfoDialog`, and non-blocking IE-style `DownloadDialog` (globe-to-folder animation, temp/custom destination, auto-open, transfer stats & timers).
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
     - `po/de.po` (German)
     - `po/es.po` (Spanish)
     - `po/fr_CA.po` (Canadian French)
     - `po/fr_FR.po` (French)
     - `po/it.po` (Italian)
     - `po/ja.po` (Japanese)
     - `po/pt.po` (Brazilian Portuguese)
     - `po/ru.po` (Russian)
     - `po/zh_CN.po` (Simplified Chinese)
- Never leave untranslated empty `msgstr ""` entries in any `.po` file, and always check for existing `msgid`s before appending to avoid duplicate-definition errors in `msgfmt`.
- Validate all catalogs after editing:
  ```sh
  for f in po/*.po; do msgfmt --check -o /dev/null "$f" || exit 1; done
  ```
