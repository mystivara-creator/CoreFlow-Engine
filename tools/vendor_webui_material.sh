#!/usr/bin/env bash
# Fetch the current upstream Material Web source at build time and vendor it
# into the assembled module. The resolved commit is recorded for provenance.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WEBROOT="${ROOT}/module/webroot"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

UPSTREAM_URL="https://github.com/material-esm/material.git"
UPSTREAM_REF="${COREFLOW_MATERIAL_REF:-fafed6f5da70f12e5d897230181f9b42dff373c6}"
UPSTREAM="${TMP}/material"
DEPS="${TMP}/deps"

command -v git >/dev/null
command -v npm >/dev/null
command -v node >/dev/null

echo "Fetching Material Web upstream ref: ${UPSTREAM_REF}"
git clone --depth 1 --branch "${UPSTREAM_REF}" "${UPSTREAM_URL}" "${UPSTREAM}"
MATERIAL_SHA="$(git -C "${UPSTREAM}" rev-parse HEAD)"
MATERIAL_VERSION="$(node -p "require('${UPSTREAM}/package.json').version")"

# The upstream source uses bare Lit imports. Install runtime packages in CI,
# then ship them beside the Material modules so the installed WebUI is offline.
npm install --prefix "${DEPS}" --no-save --omit=dev \
  lit@3 @lit/localize @lit/reactive-element@2 lit-element@4 lit-html@3

VENDOR="${WEBROOT}/vendor"
rm -rf "${VENDOR}"
mkdir -p "${VENDOR}/material" "${VENDOR}/node_modules"

# Keep upstream JS/CSS and runtime assets; omit repository-only tooling/demo.
for item in app badge buttons card carousel checkbox chips common.js dialog divider icon indicators internal labs list menu nav pickers radio search select shared slider snackbar switch tabs text tooltip typography utils all.js LICENSE; do
  if [[ -e "${UPSTREAM}/${item}" ]]; then
    cp -R "${UPSTREAM}/${item}" "${VENDOR}/material/"
  fi
done

cp -R "${DEPS}/node_modules/." "${VENDOR}/node_modules/"
cat > "${VENDOR}/material-build-info.json" <<EOF
{
  "repository": "${UPSTREAM_URL}",
  "ref": "${UPSTREAM_REF}",
  "commit": "${MATERIAL_SHA}",
  "version": "${MATERIAL_VERSION}",
  "resolved_at_utc": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "strategy": "latest upstream ref resolved and pinned per CI build"
}
EOF

# Fail closed if expected component entry points or runtime dependencies vanish.
for file in \
  "${VENDOR}/material/buttons/button.js" \
  "${VENDOR}/material/switch/switch.js" \
  "${VENDOR}/material/card/card.js" \
  "${VENDOR}/node_modules/lit/index.js" \
  "${VENDOR}/node_modules/lit-html/lit-html.js" \
  "${VENDOR}/node_modules/lit-element/lit-element.js" \
  "${VENDOR}/node_modules/@lit/reactive-element/reactive-element.js" \
  "${VENDOR}/node_modules/@lit/localize/lit-localize.js"; do
  if [[ ! -s "${file}" ]]; then
    echo "::error::Expected Material runtime asset missing: ${file}"
    exit 1
  fi
done

echo "Vendored Material ${MATERIAL_VERSION} (${MATERIAL_SHA}) into ${VENDOR}"
