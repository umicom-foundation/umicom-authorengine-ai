/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * PURPOSE: Project helper documentation.
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab
 * Date: 24-09-2025
 * License: MIT
 *---------------------------------------------------------------------------*/

# macOS Codesign & Notarisation (Intel/Apple Silicon)

## Prerequisites
- Apple Developer account + Developer ID Application certificate
- Xcode CLT: `xcode-select --install`
- App-specific password for `notarytool`

## Build
```bash
cmake --preset macos
cmake --build build -j
```

## Codesign
```bash
codesign --force --options runtime --timestamp   --sign "Developer ID Application: Your Name (TEAMID)"   build/uaengine
```

## Notarise
```bash
cd build && zip uaengine-macos.zip uaengine && cd -

xcrun notarytool store-credentials "UMICOM_NOTARY"   --apple-id "your-apple-id@example.com"   --team-id "TEAMID"   --password "app-specific-password"

xcrun notarytool submit build/uaengine-macos.zip   --keychain-profile "UMICOM_NOTARY" --wait
```

## Staple + verify
```bash
xcrun stapler staple build/uaengine
spctl --assess --verbose=4 build/uaengine
```
