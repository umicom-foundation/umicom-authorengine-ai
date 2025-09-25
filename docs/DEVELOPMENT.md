# Developer Notes

## Formatting

We use `clang-format` with the repository's `.clang-format`. CI runs a `-Werror -n` dry‑run style check.
Please run it locally on staging changes:

```powershell
git ls-files *.c *.h | %% { & "C:\Program Files\LLVM\bin\clang-format.exe" -style=file -i -- $_ }
```

## Build recipes

- **MSBuild (Visual Studio)**

  ```powershell
  cmake -S . -B build
  cmake --build build -j
  .\build\Debug\uaengine.exe --version
  ```

- **Ninja + Clang**

  ```powershell
  cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build-ninja -j
  .\build-ninja\uaengine.exe --version
  ```

