## Description

Fast Curvature Based Interpolation. More info [here](http://blog.awm.jp/tags/fcbi/) (Japanese).

This is a port of [FCBI filter](https://github.com/yoya/image.js/blob/master/fcbi.js) done by [chikuzen](https://github.com/chikuzen/FCBI) to AviSynth.

### Requirements:

- AviSynth 2.60 / AviSynth+ 3.4 or later

- Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases)) (Windows only)

### Usage:

```
FCBI(clip input, bool "ed", int "tm", int "opt")
```

### Parameters:

- input\
    A clip to process.\
    Must be in YUV 8..16-bit planar format (except YV411).

- ed\
    Use edge detection.\
    Default: False.

- tm\
    Threshold for edge detection.\
    Must be between 0 and range_max.
    Default: 30 * (2 ^ bit_depth - 1) / 255.

- opt\
    Sets which cpu optimizations to use.\
    -1: Auto-detect.\
    0: Use C++ code.\
    1: Use SSE2 code.\
    Default: -1.

### Building:

- Windows\
    Use solution files.

- Linux
    ```
    Requirements:
        - Git
        - C++17 compiler
        - CMake >= 3.16
    ```
    ```
    git clone https://github.com/Asd-g/AviSynth-FCBI && \
    cd AviSynth-FCBI && \
    mkdir build && \
    cd build && \

    cmake ..
    make -j$(nproc)
    sudo make install
    ```
