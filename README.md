Readme for Brawtool
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/brawtool/blob/master/README.md)

Introduction
------------

brawtool is a set of utilities for processing braw encoded images

Building
--------

The brawtool app can be built both from commandline or using optional Xcode `-GXcode`.

```shell
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=<path>/brawtool/modules -DCMAKE_INSTALL_PREFIX=<path> -DCMAKE_PREFIX_PATH=<path> -GXcode
cmake --build . --config Release -j 8
```

**Example using 3rdparty on arm64**

```shell
mkdir build
cd build
cmake ..
cmake .. -DCMAKE_INSTALL_PREFIX=<path>/3rdparty/build/macosx/arm64.debug -DCMAKE_INSTALL_PREFIX=<path>/3rdparty/build/macosx/arm64.debug -DCMAKE_CXX_FLAGS="-I<path>/3rdparty/build/macosx/arm64.debug/include/eigen3" -DBUILD_SHARED_LIBS=TRUE -GXcode
```

Usage
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

Packaging
---------

The `macdeploy.sh` script will deploy mac bundle to dmg including dependencies.

```shell
./macdeploy.sh -e <path>/brawtool -d <path> -p <path>
```

Dependencies
-------------

| Project     | Description |
| ----------- | ----------- |
| Boost       | [Boost project @ Github](https://github.com/boostorg/boost)
| OpenImageIO | [OpenImageIO project @ Github](https://github.com/OpenImageIO/oiio)
| OpenColorIO | [OpenColorIO project @ Github](https://github.com/AcademySoftwareFoundation/OpenColorIO)
| BlackMagic RAW     | [BlackMagic website](https://www.blackmagicdesign.com/se/products/blackmagicraw)
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