# Pip-Boy 3000

Pip-Boy 3000 is a work-in-progress Fallout-inspired interface written in C with SDL2. It includes animated STAT screens, data and inventory views backed by CSV files, audio, and an experimental map engine.

## Features

- STAT, INV, DATA, MAP, and RADIO tabs.
- Animated Vault Boy and SPECIAL screens.
- Scrollable inventory categories with weapon/ammo relationships.
- Stateful inventory actions for using Aid, equipping gear, and favoriting items.
- Versioned autosave/load with atomic writes and backup recovery.
- Quest, workshop, and player-stat views.
- Optional startup audio and a skippable boot animation.
- Bounded map panning with a lightweight fallback grid.

The planned offline country-map and GPS design is documented in [docs/OFFLINE_MAP_AND_GPS.md](docs/OFFLINE_MAP_AND_GPS.md).
The versioned persistence schema and recovery behavior are documented in [docs/SAVE_FORMAT.md](docs/SAVE_FORMAT.md).
The accumulated-radiation rules and segmented HP display are documented in [docs/RADIATION.md](docs/RADIATION.md).

## Windows quick start

The repository currently includes 64-bit MinGW SDL2 development libraries and runtime DLLs. Install GCC/MinGW, make sure `gcc` is on `PATH`, then run from PowerShell:

```powershell
./scripts/build.ps1 -Configuration Debug -RunTests
./build/PipBoy3000.exe
```

Use `--skip-boot` for a faster development launch:

```powershell
./build/PipBoy3000.exe --skip-boot
```

### Video-backed animations

All runtime frame sequences are consolidated into MPEG-1 program streams and
decoded in-process through the vendored, MIT-licensed PL_MPEG library:

- `BOOT/bootup.mpg` and `BOOT/bootboy.mpg` — full-screen, skippable startup videos.
- `STAT/vaultboy.mpg` — looping STATUS animation.
- `STAT/vaultboy-combat.mpg` — converted combat variant retained for future use.
- `STAT/{strength,perception,endurance,charisma,intelligence,agility,luck}.mpg` — complete looping SPECIAL animations.
- `RADIO/radio-waveform.mpg` — looping UNKNOWN SIGNAL waveform.

The superseded JPG, PNG, and GIF frame exports were removed after the videos
passed decoder/render tests. They remain recoverable from Git history. To
convert restored or newly exported frames, install OpenCV and run:

```powershell
python -m pip install -r scripts/requirements-video.txt
python scripts/convert_animations.py
```

MPEG-1 is the default because the application can decode it without an external
FFmpeg installation or runtime DLL. The converter can still produce archival
WebM or MP4 variants with `--format webm` or `--format mp4`.

Run the executable with the repository root as its working directory so it can find the fonts, CSV data, images, and sounds. The checked-in VS Code build and debug tasks already do this and no longer contain machine-specific paths.

## CMake build

A CMake build is also available. On Windows with MinGW:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

On Unix-like systems, CMake discovers SDL2, SDL2_image, SDL2_mixer, and SDL2_ttf through `pkg-config`.

## Controls

| Key | Action |
| --- | --- |
| `Q` / `E` | Previous / next main tab |
| `A` / `D` | Previous / next subtab |
| `W` / `S` | Move through the active list |
| `Enter` | Use selected Aid item or equip selected weapon/apparel |
| `F` | Add/remove the selected inventory item from favorites |
| Arrow keys | Pan the map while the MAP tab is active |
| `X` | Add test experience |
| `H` | Take 40 test damage |
| `Z` | Add 100 test radiation |
| Any key during boot | Skip the boot animation |

## Project layout

- `Pipboy3000.c` — application lifecycle and top-level rendering.
- `state.c` — state initialization, cleanup, XP, and damage state.
- `inventory.c` — CSV loading and inventory selection helpers.
- `core.c` — gameplay commands, item effects, equipment, favorites, and damage/radiation rules.
- `save.c` — versioned JSON persistence, autosave installation, and backup recovery.
- `resources.c` — fonts and static textures loaded once for reuse.
- `video.c` — MPEG-1 decoding, SDL YUV texture upload, looping, and blocking boot playback.
- `MAP/` — current map renderer and future offline-map integration point.
- `third_party/pl_mpeg/` — pinned MIT-licensed single-header video decoder.
- `third_party/cjson/` — pinned MIT-licensed JSON parser used for save files.
- `tests/` — regression tests for state, inventory, navigation, every video asset, and the decoder.
- `scripts/build.ps1` — dependency-free Windows build/test entry point.

## License

The source is licensed under the MIT License. See [LICENSE](LICENSE). Map data and generated country packs must retain the attribution and licensing required by their data provider.
