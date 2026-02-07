#!/bin/bash
# Script to upgrade embedded QuickJS using quickjs-ng amalgamated release.
# Usage: ./update.sh [version_tag]
# Example: ./update.sh v0.11.0

set -e

VERSION=${1:-v0.11.0}
URL="https://github.com/quickjs-ng/quickjs/releases/download/${VERSION}/quickjs-amalgam.zip"
TMP_DIR=$(mktemp -d)
DEST_DIR=$(dirname "$0")

echo "Updating QuickJS to ${VERSION}..."
echo "Downloading from ${URL}..."

if ! wget -q "${URL}" -O "${TMP_DIR}/quickjs-amalgam.zip"; then
  echo "Error: Failed to download ${URL}"
  rm -rf "${TMP_DIR}"
  exit 1
fi

echo "Extracting..."
unzip -q -o "${TMP_DIR}/quickjs-amalgam.zip" -d "${TMP_DIR}/quickjs-amalgam"

echo "Updating files in ${DEST_DIR}..."
cp "${TMP_DIR}/quickjs-amalgam/quickjs-amalgam.c" "${DEST_DIR}/quickjs.c"
cp "${TMP_DIR}/quickjs-amalgam/quickjs.h" "${DEST_DIR}/quickjs.h"

echo "Cleanup..."
rm -rf "${TMP_DIR}"

echo "Done. QuickJS updated to ${VERSION}."
