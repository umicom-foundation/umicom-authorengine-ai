$files = git ls-files *.c *.h
$clang = "clang-format"
if (-not (Get-Command $clang -ErrorAction SilentlyContinue)) { $clang = "C:\Program Files\LLVM\bin\clang-format.exe" }
$files | ForEach-Object { & $clang -i --style=file -- $_ }
