# Getting Started

Welcome! This guide walks you from zero → running **uaengine** on Windows, Linux, or macOS.

---

## Prerequisites

### Common
- CMake ≥ 3.20
- A C17-capable compiler (Clang / MSVC / GCC)

### Windows
- **Option A (fast)**: Ninja (`choco install ninja`) + LLVM/Clang
- **Option B**: Visual Studio 2022 (with C++ workload)

### Linux
```bash
sudo apt-get update && sudo apt-get install -y build-essential cmake ninja-build
