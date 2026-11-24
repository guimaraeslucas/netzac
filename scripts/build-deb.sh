#!/bin/sh
# Build the zacos9-extra-netzac Debian package from the committed tree:
#
#   scripts/build-deb.sh [VERSION]   # build/packages/zacos9-extra-netzac_<version>_<arch>.deb
#
set -eu
self=$(realpath "$0")
cd "$(dirname "$self")/.."
root=$(pwd)

die() {
	echo "build-deb.sh: $1" >&2
	exit 1
}

. "./package.conf"
version=${1:-1.0.0}
case "$version" in
	[0-9]*) ;;
	*) die "'$version' must start with a digit, e.g. 1.0.0." ;;
esac
case "$version" in
	*[!A-Za-z0-9.+~]*) die "'$version' isn't a Debian version (letters, digits, . + ~)." ;;
esac

work=$(mktemp -d /tmp/zacos9-extra-netzac-deb.XXXXXX)
trap 'rm -rf "$work"' EXIT
git archive HEAD | tar -x -C "$work" --one-top-level=src
meson setup "$work/build" "$work/src" --prefix=/usr --sysconfdir=/etc --buildtype=release >"$work/setup.log" 2>&1 ||
	{ cat "$work/setup.log" >&2; die "meson setup failed."; }
ninja -C "$work/build" >"$work/ninja.log" 2>&1 || { cat "$work/ninja.log" >&2; die "the build failed."; }
stage=$work/stage
meson install -C "$work/build" --destdir "$stage" --strip >/dev/null
[ -n "$(find "$stage" -type f)" ] || die "nothing was installed."

mkdir -p "$work/shlibs/debian"
printf 'Source: %s\n\nPackage: %s\nArchitecture: any\n' "$package" "$package" >"$work/shlibs/debian/control"
exes=$(find "$stage" -type f -perm -u+x)
shlibs=$(cd "$work/shlibs" && dpkg-shlibdeps -O $(printf '%s\n' "$exes" | sed 's|^|-e|') 2>/dev/null |
	sed -n 's/^shlibs:Depends=//p')

arch=$(dpkg --print-architecture)
mkdir -p "$stage/DEBIAN"
{
	printf 'Package: %s\nVersion: %s\nArchitecture: %s\n' "$package" "$version" "$arch"
	printf 'Maintainer: %s <%s>\n' "$(git config user.name || echo 'ZacOS 9 Contributors')" "$(git config user.email || echo 'dev@zacos9.local')"
	printf 'Section: web\nPriority: optional\n'
	printf 'Installed-Size: %s\n' "$(du -sk --exclude=DEBIAN "$stage" | cut -f1)"
	printf 'Depends: %s\n' "${shlibs:+$shlibs}${depends:+${shlibs:+, }$depends}"
	printf 'Homepage: https://github.com/guimaraeslucas/netzac\n'
	printf 'Description: %s\n' "$summary"
	printf '%s\n' "$description" | sed 's/^/ /'
} >"$stage/DEBIAN/control"

if [ -n "${programs:-}" ]; then
	{
		printf '#!/bin/sh\nset -e\n'
		printf 'if [ "$1" = remove ]; then\n'
		for p in $programs; do
			printf '\tpkill -x -f "(/usr/bin/)?%s" || true\n' "$p"
		done
		printf 'fi\n'
	} >"$stage/DEBIAN/prerm"
	chmod 755 "$stage/DEBIAN/prerm"
fi

mkdir -p "$root/build/packages"
deb=$root/build/packages/${package}_${version}_$arch.deb
dpkg-deb --root-owner-group -Zxz --build "$stage" "$deb" >/dev/null
echo "$deb"
