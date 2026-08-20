#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
public_root="$repository_root/web/public"
runtime_temp="$(mktemp -d)"
v86_version="0.5.432"
v86_commit="f3d4472a9c934b9ad78a311f5849ba711a296d23"

cleanup() {
  rm -rf "$runtime_temp"
}
trap cleanup EXIT

if [[ ! -f "$repository_root/simpleos.iso" ]]; then
  echo "Missing simpleos.iso. Run ./build.sh first."
  exit 1
fi

npm pack "v86@$v86_version" --pack-destination "$runtime_temp" >/dev/null
tar -xzf "$runtime_temp/v86-$v86_version.tgz" -C "$runtime_temp"

mkdir -p "$public_root/os" "$public_root/v86" "$public_root/bios"
install -m 0644 "$repository_root/simpleos.iso" "$public_root/os/simpleos.iso"
install -m 0644 "$runtime_temp/package/build/libv86.js" "$public_root/v86/libv86.js"
install -m 0644 "$runtime_temp/package/build/v86.wasm" "$public_root/v86/v86.wasm"

curl -fsSL \
  "https://raw.githubusercontent.com/copy/v86/$v86_commit/bios/seabios.bin" \
  --output "$public_root/bios/seabios.bin"
curl -fsSL \
  "https://raw.githubusercontent.com/copy/v86/$v86_commit/bios/vgabios.bin" \
  --output "$public_root/bios/vgabios.bin"

verify_checksum() {
  local expected="$1"
  local asset_file="$2"
  local actual
  actual="$(shasum -a 256 "$asset_file" | awk '{ print $1 }')"

  if [[ "$actual" != "$expected" ]]; then
    echo "Checksum verification failed for $asset_file."
    echo "Expected: $expected"
    echo "Actual:   $actual"
    exit 1
  fi
}

verify_checksum "730d3e4e4bd1d9c7ede4a4995f23da87261a723b3524da9a75c90e6e0610f46b" \
  "$public_root/v86/libv86.js"
verify_checksum "8a969d64cf8f64b8183fdcc3a2ccdb31ae5ae6db7c9fba49949106f574640a7b" \
  "$public_root/v86/v86.wasm"
verify_checksum "73e3f359102e3a9982c35fce98eb7cd08f18303ac7f1ba6ebfbe6cdc1c244d98" \
  "$public_root/bios/seabios.bin"
verify_checksum "a4bc0d80cc3ca028c73dafa8fee396b8d054ce87ebd8abfbd31b06b437607880" \
  "$public_root/bios/vgabios.bin"

echo "Prepared the SimpleOS ISO and pinned v86 browser assets in web/public/."
