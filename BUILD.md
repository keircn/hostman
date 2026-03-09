# Build instructions

## Dependencies

### Build-time Dependencies

| Component | Description | Notes |
|-----------|-------------|-------|
| C Compiler | GCC or Clang | C11 compatible |
| CMake | Build system | Version 3.12+ |

### Runtime Dependencies

| Library | Description | Required |
|---------|-------------|----------|
| libcurl | HTTP client library | Yes |
| cJSON | JSON parsing | Yes |
| SQLite3 | Local database storage | Yes |
| OpenSSL | Cryptography | Yes |
| ncurses | TUI support | Optional (via `HOSTMAN_USE_TUI`) |

---

## Platform-Specific Dependencies

### Linux (Arch-based)

```bash
sudo pacman -S --needed \
  base-devel \
  cmake \
  pkgconf \
  curl \
  sqlite \
  cjson \
  openssl \
  ncurses \
  clang
```

```

---

### macOS

macOS is untested, I am fairly sure it works based on some discussions with other developers but open an issue if I messed something up

**Build-time:**

```bash
brew install \
  cmake \
  curl \
  sqlite \
  cjson \
  openssl \
  ncurses \
  clang-format
```

**Note:** On macOS, OpenSSL is often installed via Homebrew. You may need to set:

```bash
export OPENSSL_ROOT_DIR=$(brew --prefix openssl)
```

---

### Windows (MSYS2/MinGW)

**Install MSYS2 first, then:**

```bash
pacman -S --needed \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-pkgconf \
  mingw-w64-x86_64-libcurl \
  mingw-w64-x86_64-sqlite3 \
  mingw-w64-x86_64-cjson \
  mingw-w64-x86_64-openssl \
  mingw-w64-x86_64-ncurses
```

---

### Windows (Visual Studio)

Install Visual Studio with C++ support, then install vcpkg for dependencies:

```bash
vcpkg install curl:x64-windows sqlite3:x64-windows cjson:x64-windows openssl:x64-windows
```

---

### FreeBSD

**Build-time:**

```bash
pkg install \
  cmake \
  pkgconf \
  curl \
  sqlite3 \
  cjson \
  openssl \
  ncurses \
  llvm
```

**Runtime:**

```bash
pkg install \
  curl \
  sqlite3 \
  cjson \
  openssl \
  ncurses
```

---

### Nix/NixOS

The project includes a `flake.nix` for declarative builds. Simply run:

```bash
nix build .
```

Or with TUI disabled:

```bash
nix build . --override-input hostman/hostman_use_tui false
```

---

### Other platforms

I can't test every platform so if you have valid dependencies and/or tweak steps for installing on any other platform, feel free to open a pull request or an issue and I'll take a look.

## CMake Build Commands

### Basic Build

```bash
git clone https://github.com/keircn/hostman && cd hostman
cmake -B build
cmake --build build
```

### Build without TUI (CLI-only)

```bash
cmake -B build -DHOSTMAN_USE_TUI=OFF
cmake --build build
```

### Release Build (Optimized)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Debug Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### Custom Installation Prefix

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

### Custom Compiler

**Using GCC:**

```bash
cmake -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

**Using Clang:**

```bash
cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

---

## CMake Options

| Option | Description | Default |
|--------|-------------|---------|
| `HOSTMAN_USE_TUI` | Build with TUI support (requires ncurses) | ON |
| `BUILD_DOCS` | Build documentation with Doxygen | OFF |
| `CMAKE_BUILD_TYPE` | Build type: Debug, Release, RelWithDebInfo, MinSizeRel | None |
| `CMAKE_INSTALL_PREFIX` | Installation directory | /usr/local |
| `CMAKE_C_COMPILER` | C compiler | System default |
| `CMAKE_EXPORT_COMPILE_COMMANDS` | Generate compile_commands.json | OFF |

---

## Additional Build Targets

### Build Documentation

Requires Doxygen installed:

```bash
cmake -B build -DBUILD_DOCS=ON
cmake --build build --target docs
```

### Code Formatting

Requires clang-format:

```bash
cmake --build build --target format
```

### Check Code Formatting

```bash
cmake --build build --target check-format
```

---

## Installation

### Standard Installation

```bash
sudo cmake --install build
```

### Custom Installation

```bash
cmake --install build --prefix ~/.local
```

### Uninstall

```bash
sudo cmake --install build --uninstall
```

---

## Troubleshooting

### OpenSSL Not Found (macOS)

If Homebrew OpenSSL is not found, explicitly set the path:

```bash
cmake -B build -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
```

### ncurses Not Found (Linux)

Ensure ncurses development packages are installed:

```bash
# Ubuntu/Debian
sudo apt-get install libncurses5-dev

# Fedora/RHEL
sudo dnf install ncurses-devel

# Arch
sudo pacman -S ncurses
```

### Static Build (Linux)

For static linking:

```bash
cmake -B build \
  -DCMAKE_EXE_LINKER_FLAGS="-static" \
  -DHOSTMAN_USE_TUI=OFF
```

---

## Cross-Compilation

### ARM64 (Linux)

```bash
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/arm-linux-gnueabihf.cmake \
  -DHOSTMAN_USE_TUI=OFF
```

### Windows (from Linux)

Using MinGW-w64 cross-compiler:

```bash
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=/usr/share/mingw/toolchain-x86_64-w64-mingw32.cmake \
  -DHOSTMAN_USE_TUI=OFF
```
