Brawtool
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/brawtool/blob/master/README.md)

Introduction
------------

Brawtool is a command-line utility for working with Blackmagic RAW (BRAW) encoded images.
It provides functionality for extracting frames, adjusting image parameters, cloning BRAW and proxy files, and generating preview images with optional LUTs and metadata applied.

![brawtool](resources/brawtool.jpg 'brawtool')

With Brawtool, you can:

- Customize output format and directory structure.
- Extract frames from BRAW files into standard image formats (e.g., EXR, JPG, PNG).
- Adjust input parameters such as white balance (Kelvin/tint) and exposure.
- Clone original BRAW files and their proxy directories to a new location.
- Generate preview images with custom resolution, applied LUTs, or embedded metadata.

General flags
-----

Print brawtool help message with flag ```--help```.

```shell
brawtool -- a set of utilities for processing braw encoded images

Usage: brawtool [options] filename...

General flags:
    --help                         Print help message
    -v                             Verbose status messages
    --inputfilename OUTFILENAME    Input filename of braw file
    --kelvin KELVIN                Input white balance kelvin adjustment
    --tint TINT                    Input white balance tint adjustment
    --exposure EXPOSURE            Input linear exposure adjustment
Output flags:
    --outputdirectory OUTFILENAME  Output directory of braw files
    --outputformat OUTFORMAT       Output format for preview image (png)
    --clonebraw                    Clone braw file to output directory
    --cloneproxy                   Clone proxy directory to output directory
    --apply3dlut                   Apply 3dlut to preview image
    --applymetadata                Apply metadata to preview image
    --override3dlut OVERRIDE3DLUT  Override 3dlut for preview image
    --width WIDTH                  Output width of preview image
    --height HEIGHT                Output height of preview image
```

Building
--------

The brawtool app can be built both from commandline or using optional Xcode `-GXcode`.

```shell
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=<path>/brawtool/modules -DCMAKE_PREFIX_PATH=<path> -GXcode
cmake --build . --config Release -j 8
```

**Example using 3rdparty on arm64 with Xcode**

```shell
mkdir build
cd build
cmake ..
cmake .. -DCMAKE_PREFIX_PATH=<path>/3rdparty/build/macosx/arm64.debug -DCMAKE_CXX_FLAGS="-I<path>/3rdparty/build/macosx/arm64.debug/include/eigen3" -GXcode
```

Download
---------

Brawtool is included as part of pipeline tools. You can download it from the releases page:

* https://github.com/mikaelsundell/pipeline/releases

Dependencies
-------------

| Project     | Description |
| ----------- | ----------- |
| Boost       | [Boost project @ Github](https://github.com/boostorg/boost)
| OpenImageIO | [OpenImageIO project @ Github](https://github.com/OpenImageIO/oiio)
| OpenColorIO | [OpenColorIO project @ Github](https://github.com/AcademySoftwareFoundation/OpenColorIO)
| Blackmagic RAW     | [Blackmagic RAW installer](https://www.blackmagicdesign.com/event/blackmagicrawinstaller)
| 3rdparty    | [3rdparty project containing all dependencies @ Github](https://github.com/mikaelsundell/3rdparty)

Project
-------

* GitHub page   
https://github.com/mikaelsundell/brawtool
* Issues   
https://github.com/mikaelsundell/brawtool/issues

Copyright
---------


* Roboto font   
https://fonts.google.com/specimen/Roboto   
Designed by Christian Robertson
