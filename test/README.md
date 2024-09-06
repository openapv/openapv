# Materials for APV codec testing

## Test bitstreams
"bitstream" folder has several encoded bitstreams for decoder conformance testing.

|No. | Bitstream Name | Description | Profile | Level | Frame Rate | Resolution | # of Frame | MD5 sum of bitstream | MD5 sum of decoded   frames|
|--|--|--|--|--|--|--|--|--|--|
|1 | tile_A | one-tile per   one-picture | Baseline Profile | 2.1 | 60 fps | 3840x2160 | 3 | 88ee04dab8722c6a25f4ffb853f517e6 | d748a94e549d47edf36f2f1a9676f694|
|2 | tile_B | Tile size = min size   tile (256x128) | Baseline Profile | 2.1 | 60 fps | 3840x2160 | 3 | 1084aa67976ed74247ff8aec786afc69 | 165e33ec2dbc487bdeabb913a3a8f964|
|3 | tile_C | # of Tiles: max num   tile (20x20) | Baseline Profile | 2.1 | 15 fps | 7680x4320 | 3 | fd3024b64e0687bd7046dced0a3dba49 | 216fb858da9de915750238d88eba002a|
|4 | tile_D | tile dummy data test | Baseline Profile | 2.1 | 60 fps | 3840x2160 | 3 | b3db44c601720eca4cd2db391f3b9f92 | 165e33ec2dbc487bdeabb913a3a8f964|
|5 | tile_E | tile_size_present_in_fh_flag=on | Baseline Profile | 2.1 | 60 fps | 3840x2160 | 3 | e9692f6d2976c43c789e35e1285830e2 | 165e33ec2dbc487bdeabb913a3a8f964|
|6 | qp_A | QP matrix enabled | Baseline Profile | 2.1 | 60 fps | 3840x2160 | 3 | 2abb7fec0d36cf21cb40a5e3d26bab27 | 74b42d44450c191a8253c68ac41d109c|
|7 | qp_B | Tile QP   variation in a frame | Baseline Profile | 2.1 | 60 fps | 3840x2160 | 3 | f650489015b7a9376914183a936b38a7 | cf8719168be128d3515f9ad0f9710490|
|8 | qp_C | Set all the QPs in a   frame equal to min. QP (=0) | Baseline Profile | 4 | 60 fps | 3840x2160 | 3 | a4cf585449614976ab2cd5bdf2dff7d9 | 4f79bfefd4d5cf4e80ca19de0dc1db55|
|9 | qp_D | Set all the QPs in a   frame equal to max. QP (=51) | Baseline Profile | 2.1 | 60 fps | 3840x2160 | 3 | 7a85a72a2808c4d29e2e454c009a2a9a | 1444a7de4d5fca1fc7b14134015fd123|
|10 | qp_E | Set different QP   betwee luma and chroma | Baseline Profile | 2.1 | 60 fps | 3840x2160 | 3 | a4d95f42d12ca63d1183d1476eeafd16 | b894e783b08be5e8aa4a84482fa48402|
|11 | syn_A | Exercise a synthetic   image with QP = 0 and QP = 51 | Baseline Profile | 2.1 | 60 fps | 1920x1080 | 2 | a0cfae4a37c0b395d3f8b92ef7f5a3e0 | e1b2a1b9d7b7830c2ea43548683d84e4|
|12 | syn_B | Exercise a synthetic   image with Tile QP variation in Frame | Baseline Profile | 2.1 | 60 fps | 1920x1080 | 2 | 595490398297385d6373e533efdb4454 | 3d8ad200103e825157ccd9266420ba96|
