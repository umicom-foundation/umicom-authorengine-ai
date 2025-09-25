cmake -S . -B build | Out-Null
cmake --build build -j | Out-Null
./build/uaengine.exe --help
exit 0
