#!/bin/bash
# GoldenCheetah Build Dependencies Installation Script
# For Debian/Ubuntu-based systems

set -e

echo "========================================="
echo "GoldenCheetah Build Dependencies Installer"
echo "========================================="
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then 
    echo "Please do not run this script as root"
    echo "It will ask for sudo password when needed"
    exit 1
fi

echo "This script will install the following:"
echo "  - Qt6 development tools"
echo "  - CMake build system"
echo "  - GNU Scientific Library (GSL)"
echo "  - Build essentials (gcc, g++, make)"
echo "  - Additional dependencies"
echo ""
read -p "Continue? (y/n) " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Installation cancelled"
    exit 1
fi

echo ""
echo "Updating package lists..."
sudo apt-get update

echo ""
echo "Installing build essentials..."
sudo apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    git \
    flex \
    bison

echo ""
echo "Installing CMake..."
sudo apt-get install -y cmake ninja-build

echo ""
echo "Installing Qt6 development packages..."
sudo apt-get install -y \
    qt6-base-dev \
    qt6-svg-dev \
    qt6-webengine-dev \
    qt6-multimedia-dev \
    qt6-serialport-dev \
    qt6-positioning-dev \
    qt6-charts-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    qt6-webchannel-dev \
    qt6-connectivity-dev \
    qt6-5compat-dev

echo ""
echo "Installing GNU Scientific Library..."
sudo apt-get install -y libgsl-dev

echo ""
echo "Installing additional dependencies..."
sudo apt-get install -y \
    zlib1g-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev

echo ""
echo "========================================="
echo "Installation Complete!"
echo "========================================="
echo ""
echo "Installed versions:"
echo "-------------------"
gcc --version | head -1
g++ --version | head -1
cmake --version | head -1
qmake6 --version 2>/dev/null || echo "qmake6: Not found (may need to set PATH)"
echo ""
echo "Next steps:"
echo "1. Configure Qt6 in your PATH if needed:"
echo "   export PATH=/usr/lib/qt6/bin:\$PATH"
echo ""
echo "2. Build with qmake:"
echo "   cd src"
echo "   qmake6 -recursive"
echo "   make"
echo ""
echo "3. Or build with CMake:"
echo "   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release"
echo "   cmake --build build --parallel"
echo ""
