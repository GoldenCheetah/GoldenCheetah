# GoldenCheetah Technology Modernization Guide

## Overview

This document describes the technology modernization efforts for GoldenCheetah, including the transition to modern C++ standards, Qt6 support, CMake build system, and improved dependency management.

## What Has Been Implemented

### 1. Qt5/Qt6 Dual Support

**File Modified**: `src/src.pro`

The build system now supports both Qt 5.15+ and Qt6:
- Updated version check to accept Qt 5.15+ or Qt 6.0+
- Conditional compilation for Qt6-specific modules (webenginequick, core5compat)
- Maintained backward compatibility with Qt5

**Benefits**:
- Users can build with either Qt5 or Qt6
- Smooth migration path to Qt6
- Access to Qt6 performance improvements and new features

### 2. C++17 Standard Adoption

**File Modified**: `src/src.pro`

Upgraded from C++11 to C++17:
- Changed `CONFIG += c++11` to `CONFIG += c++17`
- Added explicit `-std=c++17` compiler flag
- Enables modern C++ features: structured bindings, std::filesystem, constexpr if, etc.

**Benefits**:
- Better performance through compiler optimizations
- Cleaner, more maintainable code
- Access to modern standard library features

### 3. Security Hardening

**File Modified**: `src/src.pro`

Added platform-specific security flags:

**Windows (MSVC)**:
- `/GS` - Buffer security check
- `/sdl` - Additional security checks
- `_FORTIFY_SOURCE=2` - Runtime buffer overflow detection

**Linux (GCC/Clang)**:
- `-fstack-protector-strong` - Stack protection
- `-D_FORTIFY_SOURCE=2` - Runtime checks
- `-Wl,-z,relro,-z,now` - Full RELRO for binary hardening

**macOS**:
- `-fstack-protector-strong` - Stack protection

**Benefits**:
- Protection against buffer overflows
- Detection of memory corruption
- Hardened binaries against common exploits

### 4. CMake Build System

**Files Created**: 
- `CMakeLists.txt` - Root CMake configuration
- `CMakePresets.json` - Build presets for different configurations
- `INSTALL-CMAKE` - CMake build instructions

**Features**:
- Modern CMake 3.16+ configuration
- Support for Qt5 and Qt6
- Platform-specific configurations (Windows, macOS, Linux)
- Build presets: default, debug, release, vcpkg
- Security flags integrated
- Parallel build support
- CPack integration for packaging

**Benefits**:
- Better cross-platform support
- Faster builds with Ninja generator
- Easier dependency management
- Industry-standard build system
- Better IDE integration

### 5. Dependency Management

**Files Created**:
- `vcpkg.json` - vcpkg manifest for dependency management

**Features**:
- Declarative dependency specification
- Version constraints
- Optional features (python, r, libusb, libical, vlc, samplerate)
- Cross-platform package management

**Benefits**:
- Simplified dependency installation
- Reproducible builds
- Automatic dependency resolution
- Support for optional features

### 6. Code Quality Tools

**Files Created**:
- `.clang-format` - Code formatting configuration
- `.clang-tidy` - Static analysis configuration

**Features**:

**clang-format**:
- Qt-style code formatting
- 120 character line limit
- Consistent indentation and spacing
- Automatic include sorting

**clang-tidy**:
- Modern C++ checks
- Bug detection
- Performance analysis
- Code modernization suggestions
- Readability improvements

**Benefits**:
- Consistent code style across project
- Early bug detection
- Automated code quality checks
- Easier code reviews

## Migration Guide

### For Users

#### Building with qmake (Traditional Method)

The existing qmake build system continues to work as before:

```bash
cd src
cp gcconfig.pri.in gcconfig.pri
# Edit gcconfig.pri as needed
qmake -recursive
make
```

#### Building with CMake (New Method)

**Basic build**:
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

**With vcpkg**:
```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake -B build -S . --preset vcpkg
cmake --build build --parallel
```

**Platform-specific**:
```bash
# Linux
cmake -B build -S . --preset linux

# macOS
cmake -B build -S . --preset macos

# Windows
cmake -B build -S . --preset windows
```

See `INSTALL-CMAKE` for detailed instructions.

### For Developers

#### Code Modernization

When writing new code or refactoring, use C++17 features:

**Before (C++11)**:
```cpp
auto it = map.find(key);
if (it != map.end()) {
    value = it->second;
}
```

**After (C++17)**:
```cpp
if (auto it = map.find(key); it != map.end()) {
    value = it->second;
}
```

**Use structured bindings**:
```cpp
for (const auto& [key, value] : map) {
    // Use key and value directly
}
```

**Use std::filesystem**:
```cpp
#include <filesystem>
namespace fs = std::filesystem;

if (fs::exists(path)) {
    // File operations
}
```

#### Code Formatting

Format code before committing:
```bash
clang-format -i src/**/*.cpp src/**/*.h
```

Or set up your IDE to format on save.

#### Static Analysis

Run clang-tidy on modified files:
```bash
clang-tidy src/Core/MyFile.cpp -- -Isrc/Core -I/path/to/qt/include
```

Or integrate with your IDE for real-time feedback.

## Compatibility

### Backward Compatibility

- **qmake builds**: Fully supported, no changes required
- **Qt5 builds**: Fully supported (Qt 5.15+)
- **Existing code**: All existing code continues to work
- **Binary compatibility**: No ABI changes

### Forward Compatibility

- **Qt6 builds**: Supported with conditional compilation
- **C++17 features**: Can be used in new code
- **CMake builds**: Coexists with qmake

## Testing

### Build Testing

Test both build systems:
```bash
# Test qmake build
cd src
qmake -recursive
make clean && make

# Test CMake build
cd ..
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --clean-first
```

### Runtime Testing

After building with modernized configuration:
1. Test basic functionality (open athlete, import ride)
2. Test charts and analysis features
3. Test cloud synchronization
4. Test training mode
5. Test optional features (Python, R if enabled)

## Rollback Procedure

If issues arise, you can revert to the previous configuration:

### Revert qmake changes:
```bash
git checkout src/src.pro
```

### Remove CMake files:
```bash
rm CMakeLists.txt CMakePresets.json vcpkg.json INSTALL-CMAKE
rm .clang-format .clang-tidy
```

### Continue with qmake:
```bash
cd src
qmake -recursive
make
```

## Future Work

### Phase 1 (Completed)
- ✅ Qt6 support in qmake
- ✅ C++17 standard adoption
- ✅ Security hardening
- ✅ CMake build system
- ✅ vcpkg integration
- ✅ Code quality tools

### Phase 2 (Planned)
- [ ] Complete CMake migration for all modules
- [ ] Update deprecated Qt APIs
- [ ] Modernize signal/slot connections
- [ ] Add more unit tests
- [ ] CI/CD integration with CMake

### Phase 3 (Future)
- [ ] C++20 adoption
- [ ] Qt6-only features
- [ ] Performance profiling and optimization
- [ ] Memory leak detection
- [ ] Automated code modernization

## Support

### Documentation
- `INSTALL-CMAKE` - CMake build instructions
- `INSTALL-LINUX` - Linux qmake build instructions
- `INSTALL-MAC` - macOS qmake build instructions
- `INSTALL-WIN32` - Windows qmake build instructions

### Getting Help
- GitHub Issues: https://github.com/GoldenCheetah/GoldenCheetah/issues
- User Forum: https://groups.google.com/g/golden-cheetah-users
- Wiki: https://github.com/GoldenCheetah/GoldenCheetah/wiki

## Contributing

When contributing code:
1. Use C++17 features where appropriate
2. Format code with clang-format
3. Run clang-tidy and fix warnings
4. Test with both qmake and CMake
5. Test with both Qt5 and Qt6 if possible
6. Update documentation as needed

## Acknowledgments

This modernization effort maintains backward compatibility while enabling future improvements. Thanks to all contributors who helped test and refine these changes.
