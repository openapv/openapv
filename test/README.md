# Materials for APV codec testing

## Test bitstream
"bitstream" folder has the encoded bitstreams for decoder conformance testing.

|No. | Bitstream Name | Description | Profile&nbsp;&nbsp; | Level | Band | Frame Rate | Resolution | # of Frame | MD5 sum of bitstream |
|--|--|--|--|--|--|--|--|--|--|
|1 | tile_A | one-tile per   one-picture | 422-10 | 4.1 | 2 | 60 fps | 3840x2160 | 3 | 0b745f686d3154bc23a8b95b486e2c03 |
|2 | tile_B | Tile size = min size   tile (256x128) | 422-10 | 4.1 | 2 | 60 fps | 3840x2160 | 3 | c9a475186fc36cfb102638896a5d26be |
|3 | tile_C | # of Tiles: max num   tile (20x20) | 422-10 | 5 | 0 | 30 fps | 7680x4320 | 3 | 64da7cb68ec2161de5650a297e1954bb |
|4 | tile_D | tile dummy data test | 422-10 | 4.1 | 2 | 60 fps | 3840x2160 | 3 | c9a475186fc36cfb102638896a5d26be |
|5 | tile_E | tile_size_present_in_fh_flag=on | 422-10 | 4.1 | 2 | 60 fps | 3840x2160 | 3 | 2f0dc83c324876b5bf7f02be9c634cfb |
|6 | qp_A | QP matrix enabled | 422-10 | 4.1 | 2 | 60 fps | 3840x2160 | 3 | 416800a582b7cbb6a941c4c3866de60f |
|7 | qp_B | Tile QP   variation in a frame | 422-10 | 4.1 | 2 | 60 fps | 3840x2160 | 3 | 514a2aca526820009a16907ee77c3d45 |
|8 | qp_C | Set all the QPs in a   frame equal to min. QP (=0) | 422-10 | 6 | 2 | 60 fps | 3840x2160 | 3 | bc96b1acf6a2332404f712c1278f6d81 |
|9 | qp_D | Set all the QPs in a   frame equal to max. QP (=51) | 422-10 | 4.1 | 2 | 60 fps | 3840x2160 | 3 | 90f0e32577e07c30c6b5d75e709e3126 |
|10 | qp_E | Set different QP   betwee luma and chroma | 422-10 | 4.1 | 2 | 60 fps | 3840x2160 | 3 | d886c4e56086b5f53f4c87dcd62332ab |
|11 | syn_A | Exercise a synthetic   image with QP = 0 and QP = 51 | 422-10 | 4.1 | 2 | 60 fps | 1920x1080 | 2 | a8219946a3e9426935a53d6d55fce987 |
|12 | syn_B | Exercise a synthetic   image with Tile QP variation in Frame | 422-10 | 4.1 | 2 | 60 fps | 1920x1080 | 2 | a8219946a3e9426935a53d6d55fce987 |
## Test sequence
"sequence" folder has the uncompressed video sequence for encoder testing.
