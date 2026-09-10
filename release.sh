#!/bin/sh
set -eu

usage() {
    echo "Usage: $0 [--no-build] VERSION" >&2
    echo "Example: $0 0.0.5" >&2
    exit 2
}

BUILD=1
if [ "${1:-}" = "--no-build" ]; then
    BUILD=0
    shift
fi

VERSION="${1:-}"
[ -n "$VERSION" ] || usage
[ "$#" -eq 1 ] || usage

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT"

# Release archives should describe committed source. Generated ISO, LaTeX, HTML,
# and release output are ignored by git and therefore do not trip this check.
if [ "${ALLOW_DIRTY:-0}" != "1" ]; then
    if ! git diff --quiet || ! git diff --cached --quiet; then
        echo "error: working tree has tracked changes; commit/stash them first" >&2
        echo "       (set ALLOW_DIRTY=1 to override intentionally)" >&2
        exit 1
    fi
fi

BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || printf 'unknown')
if [ "$BRANCH" != "master" ]; then
    echo "warning: packaging from branch '$BRANCH' rather than master" >&2
fi

DOXYFILE="${DOXYFILE:-Doxygen-$VERSION}"
OUTROOT="$ROOT/release"
PKGNAME="VoyagerOS64-$VERSION"
STAGE="$OUTROOT/$PKGNAME"
ARCHIVE="$OUTROOT/$PKGNAME.tar.gz"
DOCNAME="VoyagerOS64-Doc-$VERSION.pdf"

if [ "$BUILD" -eq 1 ]; then
    echo "==> Building VoyagerOS ISO"
    make

    if [ ! -f "$DOXYFILE" ]; then
        echo "error: Doxygen config '$DOXYFILE' does not exist" >&2
        echo "       set DOXYFILE=/path/to/config to override" >&2
        exit 1
    fi

    command -v doxygen >/dev/null 2>&1 || {
        echo "error: doxygen is required to build release documentation" >&2
        exit 1
    }

    echo "==> Generating documentation with $DOXYFILE"
    rm -rf latex html
    doxygen "$DOXYFILE"

    echo "==> Building PDF manual"
    make -C latex
else
    echo "==> --no-build: using existing ISO and PDF artifacts"
fi

for file in VoyagerOS.iso qemu.sh README.md changelog.md bochs LICENSE.md; do
    if [ ! -f "$file" ]; then
        echo "error: required release file '$file' is missing" >&2
        exit 1
    fi
done

if [ ! -f latex/refman.pdf ]; then
    echo "error: latex/refman.pdf is missing" >&2
    echo "       rerun without --no-build or build the Doxygen PDF first" >&2
    exit 1
fi

mkdir -p "$OUTROOT"
rm -rf "$STAGE" "$ARCHIVE" "$ARCHIVE.sha256"
mkdir -p "$STAGE"

# Keep the small binary-release layout used by 0.0.4, but place it inside a
# versioned top-level directory so extraction does not scatter loose files.
cp -p VoyagerOS.iso qemu.sh README.md changelog.md bochs LICENSE.md "$STAGE/"
cp -p latex/refman.pdf "$STAGE/$DOCNAME"

printf '%s\n' "$VERSION" > "$STAGE/VERSION"

echo "==> Creating $ARCHIVE"
tar -C "$OUTROOT" -czf "$ARCHIVE" "$PKGNAME"

if command -v sha256sum >/dev/null 2>&1; then
    (cd "$OUTROOT" && sha256sum "$PKGNAME.tar.gz" > "$PKGNAME.tar.gz.sha256")
fi

echo "==> Release package ready"
echo "    $ARCHIVE"
if [ -f "$ARCHIVE.sha256" ]; then
    echo "    $ARCHIVE.sha256"
fi

echo ""
echo "Contents:"
tar -tzf "$ARCHIVE"
