Readme for logctool
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/logctool/blob/master/README.md)

Introduction
------------

brawtool a set of utilities for processing logc encoded images

![Sample image or figure.](images/image.png 'brawtool')

Building
--------

The logctool app can be built both from commandline or using optional Xcode `-GXcode`.

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
logctool -- a set of utilities for processing logc encoded images

Usage: brawtool [options] filename...

Commands that read images:
    -i FILENAME                         Input chart file
General flags:
    --help                              Print help message
    -v                                  Verbose status messages
    -d                                  Debug status messages
General flags:
    --ei EI                             LogC exposure index
    --dataformat DATAFORMAT             LogC format (float, uint10, uint16 and unit32) (default: float)
    --convertlut LUT                    LogC conversion lut
Output flags:
    --outputfilename FILE               Output filename of log steps
    --outputwidth WIDTH                 Output width of log steps
    --outputheight HEIGHT               Output height of log steps
    --outputfalsecolorcubefile FILE     Optional output false color cube (lut) file
    --outputstopscubefile FILE          Optional output stops cube (lut) file
```


Generate LogC steps in OpenEXR float
--------

```shell
./logcotol
-v
--outputwidth 2048
--outputheight 1024
--dataformat float
--outpitfilename /Volumes/Build/github/test/logctool_LogC3.exr
--outputstopscubefile /Volumes/Build/github/test/logctool_LogC3_out.cube
```

Generate Conversion LUTs in Davinci Resolve
--------

```shell
./logcotol
-v
--outputwidth 2048
--outputheight 1024
--dataformat float
--outpitfilename /Volumes/Build/github/test/logctool_LogC3.exr
--outputstopscubefile /Volumes/Build/github/test/logctool_LogC3_out.cube
```

Packaging
---------

The `macdeploy.sh` script will deploy mac bundle to dmg including dependencies.

```shell
./macdeploy.sh -e <path>/logctool -d <path> -p <path>
```

Dependencies
-------------

| Project     | Description |
| ----------- | ----------- |
| OpenImageIO | [OpenImageIO project @ Github](https://github.com/OpenImageIO/oiio)
| OpenColorIO | [OpenColorIO project @ Github](https://github.com/AcademySoftwareFoundation/OpenColorIO)
| OpenEXR     | [OpenEXR project @ Github](https://github.com/AcademySoftwareFoundation/openexr)
| 3rdparty    | [3rdparty project containing all dependencies @ Github](https://github.com/mikaelsundell/3rdparty)

Limitations
-------------

Cube LUTs are limited in their precision therefore does not handle the toe of the curve very well.

Project
-------

* GitHub page   
https://github.com/mikaelsundell/logctool
* Issues   
https://github.com/mikaelsundell/logctool/issues


Resources
---------

* ALEXA Log C Curve    
https://github.com/mikaelsundell/utilities/blob/master/whitepapers/arri/11-06-30_Alexa_LogC_Curve.pdf


Copyright
---------

* Arri references   
Copyright © 2023 ARRI AG. All rights reserved.

* Roboto font   
https://fonts.google.com/specimen/Roboto   
Designed by Christian Robertson