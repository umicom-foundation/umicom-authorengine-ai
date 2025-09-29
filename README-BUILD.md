# Umicom AuthorEngine AI – Build Quickstart (Windows)

**Author:** Sammy Hegab • **Organization:** Umicom Foundation  
**License:** MIT • **Purpose:** Make linking `libcurl` easy and restore the `uengine` target name.

## Prereqs
- Visual Studio 2022 C++ Build Tools (or full VS 2022)
- [vcpkg](https://github.com/microsoft/vcpkg) at `C:\Dev\vcpkg`
- Packages:
  ```powershell
  C:\Dev\vcpkg\vcpkg integrate install
  C:\Dev\vcpkg\vcpkg install curl[ssl]:x64-windows
  ```

## One-liner build
```powershell
cd C:\dev\umicom-authorengine-ai\scripts
.\build.ps1             # Visual Studio generator, Debug config
# or:
.\build.ps1 -UseNinja   # Ninja generator
```

This script ensures `cmake/umicom-helpers.cmake` is included and links
`CURL::libcurl` to the `uaengine` target. It also provides a convenience
target named **`uengine`** so old commands still work:

```powershell
cmake --build build --target uengine --config Debug
```

## Manual wiring (if you prefer)
1. Put this in `CMakeLists.txt` (near the bottom is fine):
   ```cmake
   include(cmake/umicom-helpers.cmake)
   ```
2. Configure with vcpkg toolchain:
   ```powershell
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
     -DCMAKE_TOOLCHAIN_FILE=C:\Dev\vcpkg\scripts\buildsystems\vcpkg.cmake `
     -DVCPKG_TARGET_TRIPLET=x64-windows
   cmake --build build --target uengine --config Debug
   ```
