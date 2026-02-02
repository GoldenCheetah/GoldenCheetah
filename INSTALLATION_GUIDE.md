# GoldenCheetah Build Dependencies Installation Guide

## Quick Start

The installation script has been fixed to use the correct package names for your system.

### Run the Installation Script

```bash
./BUILD_DEPENDENCIES_INSTALL.sh
```

When prompted, press `y` to continue with the installation.

## What Will Be Installed

The script will install:
- ✅ Qt6 development tools (base, webengine, multimedia, serialport, positioning, charts)
- ✅ CMake build system and Ninja
- ✅ GNU Scientific Library (GSL)
- ✅ Build essentials (gcc, g++, make, git, flex, bison)
- ✅ Additional dependencies (zlib, OpenGL)

## Package Name Fix Applied

**Issue**: The package `libqt6webchannel6-dev` doesn't exist in your repository.

**Solution**: Changed to `qt6-webchannel-dev` (the correct package name).

Also removed `libqt6opengl6-dev` as it's already included in `qt6-base-dev`.

## After Installation

Once the script completes successfully, you can build GoldenCheetah using either method:

### Method 1: qmake Build (Traditional + Modernized)

```bash
# Set Qt6 in PATH if needed
export PATH=/usr/lib/qt6/bin:$PATH

# Build
cd src
qmake6 -recursive
make -j$(nproc)
```

### Method 2: CMake Build (New System)

```bash
# Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --parallel

# Run
./build/src/GoldenCheetah
```

**Note**: Make sure to use `-D` prefix for CMake variables (e.g., `-DCMAKE_BUILD_TYPE=Release`, not `CMAKE_BUILD_TYPE=Release`).

## Troubleshooting

### If qmake6 is not found:
```bash
# Find Qt6 installation
dpkg -L qt6-base-dev | grep bin/qmake

# Add to PATH
export PATH=/usr/lib/qt6/bin:$PATH

# Or use full path
/usr/lib/qt6/bin/qmake -recursive
```

### If CMake can't find Qt6:
```bash
cmake -B build -S . \
  -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
```

### Check installed versions:
```bash
gcc --version
cmake --version
qmake6 --version
```

## Verification Steps

After installation, verify everything is ready:

```bash
# Check compiler
gcc --version  # Should be 7.0 or higher for C++17

# Check CMake
cmake --version  # Should be 3.16 or higher

# Check Qt6
qmake6 --version  # Should show Qt 6.x

# Check GSL
pkg-config --modversion gsl  # Should show GSL version
```

## Build Validation

Once dependencies are installed, follow these steps to validate the modernization:

### 1. Test qmake Build
```bash
cd src
qmake6 -recursive
make clean
make -j$(nproc)
```

**Expected**: Should compile with C++17 and security flags enabled.

### 2. Test CMake Build
```bash
cd ..
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

**Expected**: Should configure and build successfully.

### 3. Run Application
```bash
# From qmake build
./src/GoldenCheetah

# From CMake build
./build/src/GoldenCheetah
```

**Expected**: Application should launch and function normally.

## What's Different

### With qmake (Modernized):
- ✅ C++17 standard enabled
- ✅ Security compiler flags active
- ✅ Qt6 support (backward compatible with Qt5)
- ✅ Same build process as before

### With CMake (New):
- ✅ Modern build system
- ✅ Better dependency management
- ✅ Faster parallel builds
- ✅ Cross-platform presets
- ✅ vcpkg integration ready

## Support

If you encounter issues:
1. Check [`VALIDATION.md`](VALIDATION.md) for detailed validation steps
2. See [`MODERNIZATION.md`](MODERNIZATION.md) for migration guide
3. Refer to [`INSTALL-CMAKE`](INSTALL-CMAKE) for CMake-specific instructions
4. Check [`INSTALL-LINUX`](INSTALL-LINUX) for traditional qmake instructions

## Rollback

If you need to revert the modernization:
```bash
git checkout src/src.pro
rm CMakeLists.txt CMakePresets.json vcpkg.json
cd src && qmake -recursive && make
```

The project will work exactly as it did before the modernization.
