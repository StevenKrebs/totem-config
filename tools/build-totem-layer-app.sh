#!/bin/sh
set -eu

app=${1:?usage: build-totem-layer-app.sh /path/to/Totem Layer Reader.app}
source_dir=$(dirname "$0")
mkdir -p "$app/Contents/MacOS"
swiftc -O "$source_dir/totem-layer.swift" -o "$app/Contents/MacOS/totem-layer"
cat > "$app/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "https://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key><string>totem-layer</string>
    <key>CFBundleIdentifier</key><string>com.totem.layer.reader</string>
    <key>CFBundleName</key><string>Totem Layer Reader</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>LSBackgroundOnly</key><true/>
    <key>NSBluetoothAlwaysUsageDescription</key>
    <string>Read the active layer from your Totem keyboard.</string>
</dict>
</plist>
PLIST
codesign --force --sign - "$app"
