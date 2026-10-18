#!/usr/bin/env bash
# Vendors the WebUI's Material 3 web components (material-esm/material) and the Lit runtime
# into module/webroot/vendor/. The module ships fully offline, so nothing is loaded from a CDN
# on the device.
#
# Environment:
#   MATERIAL_REF   git ref of material-esm/material to vendor (default: main). For reproducible
#                  releases, set this to a commit SHA.
#   LIT_VERSION    major Lit line (default: 3). Lit-element 4 and reactive-element 2 match it.
#
# Requires: bash, curl, tar, npm (Node 18+). Does not need git.
set -euo pipefail

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
WEB="$ROOT/module/webroot"
VENDOR="$WEB/vendor"

MATERIAL_REPO="material-esm/material"
MATERIAL_REF="${MATERIAL_REF:-main}"
LIT_VERSION="${LIT_VERSION:-3}"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "vendor: material-esm/material@${MATERIAL_REF}"
curl -fsSL --retry 3 "https://codeload.github.com/${MATERIAL_REPO}/tar.gz/${MATERIAL_REF}" -o "$TMP/material.tgz"
mkdir -p "$TMP/material"
tar -xzf "$TMP/material.tgz" -C "$TMP/material" --strip-components=1

rm -rf "$VENDOR"
mkdir -p "$VENDOR/material" "$VENDOR/npm"

# Copy the component library, leaving out demo, test and repository tooling.
(
  cd "$TMP/material"
  tar -cf - --exclude='./demo' --exclude='./tests' --exclude='./.github' --exclude='./.idx' \
      --exclude='./docs' --exclude='./.assetsignore' --exclude='./wrangler.json' \
      --exclude='./Makefile' --exclude='./jsconfig.json' --exclude='./package*.json' \
      --exclude='./index.html' --exclude='./.gitignore' .
) | tar -xf - -C "$VENDOR/material"

echo "vendor: lit@${LIT_VERSION} runtime"
(
  cd "$TMP"
  npm init -y >/dev/null
  npm install --no-audit --no-fund --omit=dev \
    "lit@${LIT_VERSION}" "lit-element@4" "lit-html@${LIT_VERSION}" \
    "@lit/reactive-element@2" "@lit/localize"
)
for pkg in lit lit-element lit-html @lit/reactive-element @lit/localize; do
  mkdir -p "$(dirname -- "$VENDOR/npm/$pkg")"
  cp -R "$TMP/node_modules/$pkg" "$VENDOR/npm/$pkg"
done

# Every path the import map and the entry module depend on must exist.
for required in \
  "$VENDOR/material/all.js" \
  "$VENDOR/npm/lit/index.js" \
  "$VENDOR/npm/lit-html/lit-html.js" \
  "$VENDOR/npm/lit-element/lit-element.js" \
  "$VENDOR/npm/@lit/reactive-element/reactive-element.js" \
  "$VENDOR/npm/@lit/localize/lit-localize.js"
do
  [ -s "$required" ] || { echo "vendor: missing $required" >&2; exit 1; }
done

LIT_RESOLVED="$(node -p "require('$TMP/node_modules/lit/package.json').version")"
cat > "$VENDOR/VENDOR.txt" <<EOF
Vendored for the CoreFlow KernelSU WebUI. Do not edit by hand; rerun tools/vendor_webui_deps.sh.
material-esm/material: ${MATERIAL_REF}
lit: ${LIT_RESOLVED} (major ${LIT_VERSION})
license: material-esm/material is Apache-2.0; see material/LICENSE
EOF

echo "vendor: ok -> ${VENDOR#"$ROOT"/}"
