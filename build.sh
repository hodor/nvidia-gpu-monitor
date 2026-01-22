#!/bin/bash
set -e

# GPU Monitor - Linux Build Script

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}GPU Monitor - Linux Build${NC}"
echo "=========================="

# Check for required dependencies
check_dependency() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${RED}Error: $1 is not installed${NC}"
        return 1
    fi
    return 0
}

check_dependency cmake
check_dependency make

# Prefer Clang, fall back to GCC
if check_dependency clang && check_dependency clang++; then
    export CC=clang
    export CXX=clang++
    echo -e "Compiler: ${YELLOW}Clang${NC}"
elif check_dependency gcc && check_dependency g++; then
    export CC=gcc
    export CXX=g++
    echo -e "Compiler: ${YELLOW}GCC${NC}"
else
    echo -e "${RED}Error: No C++ compiler found. Install clang or gcc.${NC}"
    exit 1
fi

# Check for GLFW3
if ! pkg-config --exists glfw3 2>/dev/null; then
    echo -e "${YELLOW}Warning: GLFW3 not found. Install with:${NC}"
    echo "  sudo apt install libglfw3-dev    # Debian/Ubuntu"
    echo "  sudo dnf install glfw-devel      # Fedora"
    echo "  sudo pacman -S glfw              # Arch"
    exit 1
fi

# Build type (default: Release)
BUILD_TYPE="${1:-Release}"
echo -e "Build type: ${YELLOW}${BUILD_TYPE}${NC}"

# Create build directory and generate
echo -e "\n${GREEN}Configuring...${NC}"
cmake -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# Build
echo -e "\n${GREEN}Building...${NC}"
cmake --build build -j"$(nproc)"

echo -e "\n${GREEN}Build complete!${NC}"
echo -e "Run with: ${YELLOW}./build/gpu_monitor${NC}"
