#!/usr/bin/env bash
# Build a Nordstjernen Flatpak bundle. Drives flatpak-builder against the
# GNOME runtime using data/packaging/org.nordstjernen.WebBrowser.yml and exports
# a single-file dist/nordstjernen-<version>-<arch>.flatpak that installs with
# `flatpak install --user <file>`. See docs/flatpak.md.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
VERSION=$(grep -E "^[[:space:]]*version" "$ROOT/meson.build" | head -1 \
          | sed -E "s/.*version: '([^']+)'.*/\1/")
ARCH=$(uname -m)
NAME=nordstjernen
APPID=org.nordstjernen.WebBrowser
MANIFEST="$ROOT/data/packaging/${APPID}.yml"
RUNTIME_VERSION=$(grep -E "^runtime-version:" "$MANIFEST" \
                  | sed -E "s/.*'([^']+)'.*/\1/")

BUILDDIR="$ROOT/build-flatpak"
REPO="$ROOT/build-flatpak-repo"
STATEDIR="$ROOT/build-flatpak-state"

if command -v flatpak-builder >/dev/null 2>&1; then
    FB=(flatpak-builder)
elif flatpak info org.flatpak.Builder >/dev/null 2>&1; then
    FB=(flatpak run org.flatpak.Builder)
else
    echo "flatpak-builder not found. Install it, e.g.:" >&2
    echo "    sudo apt install flatpak-builder   # Debian/Ubuntu" >&2
    echo "    sudo dnf install flatpak-builder    # Fedora/RHEL" >&2
    echo "    flatpak install flathub org.flatpak.Builder  # any distro" >&2
    exit 1
fi

echo "Ensuring GNOME ${RUNTIME_VERSION} runtime + SDK are installed (user)..."
flatpak remote-add --user --if-not-exists flathub \
    https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user --noninteractive flathub \
    "org.gnome.Platform//${RUNTIME_VERSION}" \
    "org.gnome.Sdk//${RUNTIME_VERSION}"

rm -rf "$BUILDDIR" "$REPO"
mkdir -p "$ROOT/dist"

"${FB[@]}" \
    --force-clean \
    --state-dir "$STATEDIR" \
    --repo "$REPO" \
    "$BUILDDIR" \
    "$MANIFEST"

BUNDLE="$ROOT/dist/${NAME}-${VERSION}-${ARCH}.flatpak"
rm -f "$BUNDLE"
flatpak build-bundle "$REPO" "$BUNDLE" "$APPID"

echo
echo "Built: $BUNDLE ($(du -h "$BUNDLE" | cut -f1))"
echo
echo "Install: flatpak install --user \"$BUNDLE\""
echo "Run:     flatpak run $APPID https://example.com"
