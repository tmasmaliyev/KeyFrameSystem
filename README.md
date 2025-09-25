# Keyframe Motion Control System

A modern OpenGL-based keyframe animation system implemented in C++ for computer animation coursework. Features quaternion and Euler angle interpolation with both Catmull-Rom and B-spline curves.

## Table of Contents
* [Description](#description)
* [Features](#features)
* [Requirements](#requirements)
* [How to Build](#how-to-build)
   * [Linux](#linux)
   * [Windows](#windows)
* [How to Use](#how-to-use)
   * [Command Line Options](#command-line-options)
   * [Interactive Controls](#interactive-controls)
   * [Examples](#examples)
* [Project Structure](#project-structure)

## Description

This project implements an optimized keyframe motion control system using modern OpenGL. It demonstrates fundamental computer animation concepts including:

- Keyframe interpolation with multiple spline types
- Quaternion vs. Euler angle rotations
- Real-time 3D rendering
- Interactive camera controls
- OBJ model loading

## Features

- **Dual Rotation Systems**: Support for both quaternion and Euler angle representations
- **Multiple Interpolation Methods**: Catmull-Rom and B-spline interpolation
- **Real-time Rendering**: Modern OpenGL 3.3+ with embedded shader support
- **Interactive Controls**: Runtime switching between animation modes
- **3D Model Support**: OBJ file loading with automatic normal generation
- **Performance Monitoring**: Built-in FPS and timing statistics
- **Cross-platform**: Supports Linux, Windows

## Requirements

### Build Dependencies
- **CMake** (3.10+)
- **C++ Compiler** supporting C++17 (GCC 11+, MSVC 2017+)

### Runtime Libraries
- **OpenGL** 3.3+
- **GLFW3** (window management)
- **GLEW** (OpenGL extension loading)
- **GLM** (mathematics library)

### Platform-Specific
- **Linux**: X11 development libraries (or Wayland with X11 compatibility)
- **Windows**: Visual Studio 2017+ or MinGW-w64

## How to Build

### Linux

Install dependencies:
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake pkg-config
sudo apt install libglfw3-dev libglew-dev libglm-dev

# Fedora
sudo dnf install cmake gcc-c++ pkgconfig
sudo dnf install glfw-devel glew-devel glm-devel
```

Build the project:
```bash
mkdir build
cmake --preset win-vcpkg
cmake --build build --config Release
```

### Windows

Using Visual Studio:
```cmd
cmake --preset win-vcpkg
cmake --build build --config Release
```

## How to Use

### Command Line Options

```
./keyframe_system [OPTIONS]

Options:
  -ot <type>       Orientation type:
                   • quat/quaternion/0 (default) - Use quaternions
                   • euler/1 - Use Euler angles
                   
  -it <type>       Interpolation type:
                   • crspline/catmullrom/0 (default) - Catmull-Rom splines
                   • bspline/1 - B-spline interpolation
                   
  -kf <keyframes>  Custom keyframes in format: "x,y,z:rx,ry,rz;..."
                   Position (x,y,z) and rotation (rx,ry,rz) in degrees
                   
  -m <filepath>    3D model file (.obj format)
                   Default: cube or teapot.obj if present
                   
  -h, --help       Show help message
```

### Interactive Controls

| Key | Action |
|-----|--------|
| **Q** | Toggle between Quaternions and Euler angles |
| **S** | Toggle between B-spline and Catmull-Rom interpolation |
| **R** | Reset animation to beginning |
| **C** | Reset camera to default position |
| **P** | Toggle performance statistics display |
| **ESC** | Exit application |

**Mouse Controls:**
- **Left Click + Drag**: Rotate camera around target
- **Scroll Wheel**: Zoom in/out
- **Window Resize**: Automatically adjusts viewport

### Examples

Basic usage:
```bash
# Run with default cube and keyframes
./keyframe_system

# Use quaternions with B-spline interpolation
./keyframe_system -ot quat -it bspline

# Load custom model
./keyframe_system -m models/teapot.obj
```

Custom keyframes:
```bash
# Simple square path
./keyframe_system -kf "0,0,0:0,0,0;5,0,0:0,90,0;5,5,0:0,180,0;0,5,0:0,270,0;0,0,0:0,360,0"

# Complex 3D motion with rotations
./keyframe_system -kf "0,0,0:0,0,0;3,2,1:45,90,0;0,4,2:90,180,45;-2,1,0:135,270,90;0,0,0:180,360,135"
```

## Performance Notes

The system includes several optimizations for smooth animation:
- **Segment precomputation** for spline calculations
- **Smart caching** to avoid redundant matrix calculations
- **Binary search** for keyframe lookup
- **Embedded shaders** for faster loading

Enable performance monitoring by pressing `P` during runtime to see detailed timing information.

## Troubleshooting

**Linux Wayland Issues**: If running on Wayland, the system automatically handles X11 compatibility. If you encounter issues, ensure X11 libraries are installed.

**Shader Loading**: The system embeds shaders at compile time and falls back to file loading. If shader files are missing, check the `assets/shaders/` directory.

**Model Loading**: For custom OBJ files, ensure they contain valid geometry. The system will fall back to a default cube if loading fails.
