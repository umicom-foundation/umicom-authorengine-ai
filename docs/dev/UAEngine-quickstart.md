# UAEngine — Quickstart (Windows / Linux / macOS)

## Windows (Clang + CMake + Ninja)
```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
.uild\uaengine.exe --help
```

## Linux (clang or gcc)
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/uaengine --help
```

## macOS (Homebrew llvm + ninja)
```bash
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/uaengine --help
```
