![OAPV](/readme/img/oapv_logo_bar_256.png)
# OpenAPV (Open Advanced Professional Video Codec)

[![Build & test](https://github.com/cpncf/apv/actions/workflows/build.yml/badge.svg)](https://github.com/cpncf/apv/actions/workflows/build.yml)

OpenAPV provides the reference implementation of the [APV codec](#apv-codec) which can be used to record professional-grade video and associated metadata without quality degradation. OpenAPV is free and open source software provided by [LICENSE](#license).

The OpenAPV supports the following features:

- fully compliant with 422-10 and 400-10 profile of [APV codec](#apv-codec)
- Low complexity by optimization for ARM NEON and x86(64bit) SEE/AVX CPU
- Supports tile-based multi-threading
- Supports Various metadata including HDR10/10+ and user-defined format
- Constant QP (CQP), average bitrate (ABR), and constant rate factor (CRF) are supported


## APV codec
The APV codec is a professional video codec, which was developed in response to the need for professional level high quality video recording and post production. The primary purpose of the APV codec is for use in professional video recording and editing workflows for various types of content. 

APV codec utilizes technologies known to be over 20 years to achieve a royalty free codec. APV builds a video codec using only conventional coding technologies, which consist of traditional methods published between the early 1980s and the end of the 1990s.

The APV codec standard has the following features:

- Perceptually lossless video quality, which is close to raw video quality
- Low complexity and high throughput intra frame only coding without pixel domain prediction
- Support for high bit-rate range up to a few Gbps for 2K, 4K and 8K resolution content, enabled by a lightweight entropy coding scheme
- Frame tiling for immersive content and for enabling parallel encoding and decoding
- Support for various chroma sampling formats from 4:2:2 to 4:4:4, and bit-depths from 10 to 16
- Support for multiple decoding and re-encoding without severe visual quality degradation
- Support multi-view video and auxiliary video like depth, alpha, and preview
- Support various metadata including HDR10/10+ and user-definded format

### Related specification
- APV Codec (bitstream): [https://datatracker.ietf.org/doc/draft-lim-apv/](https://datatracker.ietf.org/doc/draft-lim-apv/)
  - Scope of OpenAPV project
- APV RTP payload format: [https://datatracker.ietf.org/doc/draft-lim-rtp-apv/](https://datatracker.ietf.org/doc/draft-lim-rtp-apv/)


## How to build
- Build Requirements
  - CMake (download from [https://cmake.org/](https://cmake.org/))
  - GCC
- Build Instructions
  ```
  cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
  cmake --build build
  ```
- Output Location
  - Executable applications can be found under build/bin/
  - Library files can be found under build/lib/

## How to use
### Encoder

Encoder as input require raw YUV file (422, 444), 10-bit or more.

Displaying help:

    $oapv_app_enc --help

Encoding:

    $oapv_app_enc -i input_1920x1080_yuv422_10bit.yuv -w 1920 -h 1080 -d 10 -z 30 -o encoded.apv

### Decoder

Decoder output can be in yuv or y4m formats.

Displaying help:

    $oapv_app_dec --help

Decoding:

    $oapv_app_dec -i encoded.apv -o output.y4m

## Testing

In build directory run:
```
ctest
```

## Packaging

For generating package ready for distribution (default deb) execute in build directory:
```
cpack
```
or other formats (tgz, zip etc.):
```
cpack -G TGZ
```

## License

See [LICENSE](LICENSE) file for details.

