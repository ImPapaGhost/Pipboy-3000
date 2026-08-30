# PL_MPEG

This directory contains `pl_mpeg.h` from
[phoboslab/pl_mpeg](https://github.com/phoboslab/pl_mpeg), pinned to commit
`c871f2be022ece7ef4f64230b4fb8e1fb9eb6023`.

PL_MPEG is a single-file MPEG-1 video and MP2 audio decoder written by Dominic
Szablewski and distributed under the MIT license. Pipboy-3000 uses its
video-only API to decode local MPEG program streams into SDL2 YUV textures.

The upstream project is not otherwise modified. The implementation is enabled
from `video.c` by defining `PL_MPEG_IMPLEMENTATION` once before including the
header.
