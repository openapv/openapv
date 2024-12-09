# Materials for APV codec testing

## Test bitstream
"bitstream" folder has the encoded bitstreams for decoder conformance testing.

| No. | Bitstream Name | Description                                                  | Profile&nbsp;&nbsp; | Level | Band | Frame Rate | Resolution | # of Frame | MD5 sum of bitstream             |
|-----|----------------|--------------------------------------------------------------|---------------------|-------|------|------------|------------|------------|----------------------------------|
| 1   | tile_A         | one-tile per   one-picture                                   | 422-10              | 4.1   | 2    | 60 fps     | 3840x2160  | 3          | b6e1cef839381b2c90cb9ffcdf537d77 |
| 2   | tile_B         | Tile size = min size   tile (256x128)                        | 422-10              | 4.1   | 2    | 60 fps     | 3840x2160  | 3          | 9a0cb5126d705b03a2e7bcdcbacf6fbf |
| 3   | tile_C         | # of Tiles: max num   tile (20x20)                           | 422-10              | 5     | 0    | 30 fps     | 7680x4320  | 3          | 75363d036965a9dccc90a9ce8d0ae652 |
| 4   | tile_D         | tile dummy data test                                         | 422-10              | 4.1   | 2    | 60 fps     | 3840x2160  | 3          | 0394e3ac275e2bc595c07c5290dc9466 |
| 5   | tile_E         | tile_size_present_in_fh_flag=on                              | 422-10              | 4.1   | 2    | 60 fps     | 3840x2160  | 3          | fdf72572b6551bc6a9eed7f80ca0ec0f |
| 6   | qp_A           | QP matrix enabled                                            | 422-10              | 4.1   | 2    | 60 fps     | 3840x2160  | 3          | 5ca6d4ea0f65add261b44ed3532a0a73 |
| 7   | qp_B           | Tile QP   variation in a frame                               | 422-10              | 4.1   | 2    | 60 fps     | 3840x2160  | 3          | 1b24d4f97c18545b7881002cc642839b |
| 8   | qp_C           | Set all the QPs in a   frame equal to min. QP (=0)           | 422-10              | 6     | 2    | 60 fps     | 3840x2160  | 3          | 8c2928ec05eb06d42d6a8bda0ceb7e8d |
| 9   | qp_D           | Set all the QPs in a   frame equal to max. QP (=51)          | 422-10              | 4.1   | 2    | 60 fps     | 3840x2160  | 3          | e5acd3d3a0aa7bd6a45a49af35980512 |
| 10  | qp_E           | Set different QP   betwee luma and chroma                    | 422-10              | 4.1   | 2    | 60 fps     | 3840x2160  | 3          | e58ea5df35750c0d19cffefde12e78c4 |
| 11  | syn_A          | Exercise a synthetic   image with QP = 0 and QP = 51         | 422-10              | 4.1   | 2    | 60 fps     | 1920x1080  | 2          | e1593a670c62d69718986ff84d1150f3 |
| 12  | syn_B          | Exercise a synthetic   image with Tile QP variation in Frame | 422-10              | 4.1   | 2    | 60 fps     | 1920x1080  | 2          | 9f188e39824829aa05db584034ab1fd0 |

## Test sequence
"sequence" folder has the uncompressed video sequence for encoder testing.
