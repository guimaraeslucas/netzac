#!/bin/sh
# Install zacos9-netzac for the current user (~/.local/bin, ~/.local/share)
# completely separate from the global ZacOS 9 system.
#   scripts/install-user.sh
set -eu
cd "$(dirname "$0")/.."

if [ ! -d build ]; then
	meson setup build --prefix="$HOME/.local"
fi
ninja -C build
ninja -C build zacos9-netzac-gmo || true

bin="$HOME/.local/bin"
apps="$HOME/.local/share/applications"
icons="$HOME/.local/share/icons/hicolor/scalable/apps"
locale="$HOME/.local/share/locale"

install -Dm755 build/shell/netzac/zacos9-netzac "$bin/zacos9-netzac"
mkdir -p "$apps" "$icons"
rm -f "$apps/zacos9-netzac.desktop" "$icons/zacos9-netzac.svg"
install -Dm644 share/icons/hicolor/scalable/apps/netzac.svg "$icons/netzac.svg"
sed "s|^Exec=.*|Exec=$bin/zacos9-netzac %u|" share/applications/netzac.desktop >"$apps/netzac.desktop"
update-desktop-database "$apps" 2>/dev/null || true
gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" 2>/dev/null || true

for mo in build/po/*/LC_MESSAGES/zacos9-netzac.mo; do
	[ -f "$mo" ] || continue
	lang=$(basename "$(dirname "$(dirname "$mo")")")
	install -Dm644 "$mo" "$locale/$lang/LC_MESSAGES/zacos9-netzac.mo"
done

echo "Installed zacos9-netzac to $bin and desktop entry to $apps."
