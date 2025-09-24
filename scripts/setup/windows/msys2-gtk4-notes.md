# GTK4 via MSYS2 (UCRT64)

1) Open **MSYS2 UCRT64** shell.
2) Update:
```
pacman -Syu
# reopen shell
pacman -Su
```
3) Install toolchain & GTK4:
```
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain                      mingw-w64-ucrt-x86_64-gtk4                      mingw-w64-ucrt-x86_64-libadwaita                      mingw-w64-ucrt-x86_64-meson                      mingw-w64-ucrt-x86_64-ninja                      mingw-w64-ucrt-x86_64-cmake                      mingw-w64-ucrt-x86_64-pkg-config
```
