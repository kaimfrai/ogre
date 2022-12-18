# Purpose of this fork
This fork was used to implement and measure several C++ features ranging from C++11 to C++23. It requires Clang 14 for the first few modernizations and a specific preview version of Clang 15 for the most modern features. In particular, it requires LLVM built from the commit https://github.com/llvm/llvm-project/commit/9a04710b57fe with the changes from https://reviews.llvm.org/D120634 (source_location) applied. This stems from the fact that the handling of modules was changed afterwards and source_location was used to replace certain macros while modernizing.

The fork was minimized before any modernization took place to reduce the amount of work required. With several parts of the original OGRE missing, it can't be simply merged into another version of OGRE. The remaining parts only support one platform: Linux (Tested on Kubuntu 22.04).

The measuring of features was conducted as follows, once before and once after the feature was applied:
### Compilation Duration
> Evaluation/compilation_time.sh ./ Debug 5
> Evaluation/compilation_time.sh ./ Release 5
> Evaluation/assembly_generation.sh ./ Release [C++ standard number] [Path to Clang]

### Performance
> mkdir build-performance
> cmake -G Ninja --toolchain CMake/toolchain/linux.toolchain.clang.cmake -DCMAKE_BUILD_TYPE=Release -DOGRE_DEPENDENCIES_DIR=../ThirdParty/ -S ./  -B ./build-performance -DBUILD_WITH_SANITIZER=OFF -DOGRE_TRACK_MEMORY=OFF
> cmake --build ./build-performance

To get frame time metrics:
> build-performance/bin/SampleBrowser 666

To get the startup time:
> time build-performance/bin/SampleBrowser 0

### Memory
> mkdir build-memory
> cmake -G Ninja --toolchain CMake/toolchain/linux.toolchain.clang.cmake -DCMAKE_BUILD_TYPE=Release -DOGRE_DEPENDENCIES_DIR=../ThirdParty/ -S ./  -B ./build-memory -DBUILD_WITH_SANITIZER=OFF -DOGRE_TRACK_MEMORY=ON
> cmake --build ./build-memory

To get the most accurate measurements, don't do any input such as moving the mouse or typing while the program is running. Any input may lead to measurable deviations in consumed memory. Memory consumption was observed to stabilize after approximately 20 frames.
> build-memory/bin/SampleBrowser 0
> build-memory/bin/SampleBrowser 20
> build-memory/bin/SampleBrowser 666

# Original README

[![GitHub release](https://img.shields.io/github/release/ogrecave/ogre.svg)](https://github.com/OGRECave/ogre/releases/latest)
[![Join the chat at https://gitter.im/OGRECave/ogre](https://badges.gitter.im/OGRECave/ogre.svg)](https://gitter.im/OGRECave/ogre?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Patreon](https://img.shields.io/badge/patreon-donate-blue.svg)](https://www.patreon.com/ogre1)

![](Other/ogre_header.png)

## Summary
**OGRE** (Object-Oriented Graphics Rendering Engine) is a
scene-oriented, flexible 3D engine written in C++ designed to make it
easier and more intuitive for developers to produce games and demos
utilising 3D hardware. The class library abstracts all the details of
using the underlying system libraries like Direct3D and OpenGL and
provides an interface based on world objects and other intuitive
classes.

| Build | Status |
|-------|-----------------|
| Linux, OSX, Android, iOS | [![CI Build](https://github.com/OGRECave/ogre/workflows/CI%20Build/badge.svg?branch=master)](https://github.com/OGRECave/ogre/actions?query=branch%3Amaster) |
| MSVC | [![Build status](https://ci.appveyor.com/api/projects/status/kcki7y0n1ahrggdw/branch/master?svg=true)](https://ci.appveyor.com/project/paroj/ogre-bsrh7/branch/master) |

## Index Of Contents
* [What's New?](Docs/13-Notes.md)
A summary of the new and altered features in this release.
* [Building the core OGRE libraries](https://ogrecave.github.io/ogre/api/latest/building-ogre.html)  
If you're using the full source release, this will help you build it. If you're using a precompiled SDK then most of the work has already been done for you, and you should use the sample projects to see how to compile your own code against OGRE.
* [The OGRE Manual](https://ogrecave.github.io/ogre/api/latest/manual.html)  
A high-level guide to the major parts of the engine and script reference.
* [API Reference](https://ogrecave.github.io/ogre/api/latest/)  
The full OGRE API documentation, as generated from the (heavily!) commented source.
* [The OGRE Tutorials](https://ogrecave.github.io/ogre/api/latest/tutorials.html)  
A gold mine of tutorials, tips and code snippets which will help you get up to speed with the engine.

## Try it
* [Online Emscripten Demo](https://ogrecave.github.io/ogre/emscripten/)
* [Linux Snap Package](https://snapcraft.io/ogre)
* [Android App on F-Droid](https://f-droid.org/packages/org.ogre.browser/)

## Features

For an exhaustive list, see the [features page](http://www.ogre3d.org/about/features) and try our Sample Browser. For a quick overview see below

| Integrated Bump Mapping | Integrated shadows |
|----|----|
| ![](Other/screenshots/bumpmap.jpg) | ![](Other/screenshots/shadows.jpg) |


| HW & SW skeletal animation | Multi-layer Terrain |
|----|----|
| ![](Other/screenshots/skeletal.jpg) | ![](Other/screenshots/terrain.jpg) |

| Automatic Rendertarget pipelining (Compositors) | Volume Rendering with CSG & Triplanar Texturing |
|----|----|
| ![](Other/screenshots/compositor.jpg) | ![](Other/screenshots/volume.jpg) |

| [Dear ImGui](https://github.com/ocornut/imgui) | Particle Effects |
|----|----|
| ![](Other/screenshots/imgui.jpg) | ![](Other/screenshots/particle.jpg) |

## Who is using it?

**Open Source**
- [Rigs of Rods - Soft Body Physics Simulator](https://rigsofrods.org/)
- [Gazebo - Robot simulation](http://gazebosim.org/)
- [OpenCV OVIS visualization module](https://docs.opencv.org/master/d2/d17/group__ovis.html)
- [ROS 3D visualization tool](http://wiki.ros.org/rviz)
- [RAISIM Physics](https://github.com/raisimTech/raisimOgre#news)

**Closed Source**
- [Hob](http://store.steampowered.com/app/404680/Hob/)
- [Torchlight II](http://store.steampowered.com/app/200710/Torchlight_II/)
- [Battlezone 98 Redux](http://store.steampowered.com/app/301650/Battlezone_98_Redux/)

## Contributing
We welcome all contributions to OGRE, be that new
plugins, bugfixes, extensions, tutorials, documentation, example
applications, artwork or pretty much anything else! If you would like
to contribute to the development of OGRE, please create a [pull request](https://github.com/OGRECave/ogre/pulls).

## Getting Support
Please use our [community support forums](http://forums.ogre3d.org/) if you need help or
think you may have found a bug.

## Licensing
Please see the [full license documentation](Docs/License.md) for details.
