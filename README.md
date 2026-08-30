# Pip-Boy 3000

Pip-Boy 3000 is a work-in-progress Fallout-inspired interface written in C with SDL2. It includes animated STAT screens, data and inventory views backed by CSV files, audio, and an experimental map engine.

## Features

- STAT, INV, DATA, MAP, and RADIO tabs.
- Animated Vault Boy and SPECIAL screens.
- Scrollable inventory categories with weapon/ammo relationships.
- Quest, workshop, and player-stat views.
- Optional startup audio and a skippable boot animation.
- Bounded map panning with a lightweight fallback grid.

The planned offline country-map and GPS design is documented in [docs/OFFLINE_MAP_AND_GPS.md](docs/OFFLINE_MAP_AND_GPS.md).

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

### Boot animation videos

The two numbered JPEG sequences are also provided as VP9 WebM videos:

- `BOOT/bootup.webm` — 120 frames at 12.5 FPS (80 ms per source frame).
- `BOOT/bootboy.webm` — 15 frames at 8.333 FPS (120 ms per source frame).

Regenerate them with Python and OpenCV:

```powershell
python -m pip install -r scripts/requirements-video.txt
python scripts/convert_animations.py
```

Use `--format mp4` to generate MPEG-4 Part 2 MP4 alternatives. The application currently retains the JPEG player as its dependency-free fallback; switching runtime playback to the videos requires adding a video decoder because SDL2 does not decode video containers by itself.

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
| Arrow keys | Pan the map while the MAP tab is active |
| `X` | Add test experience |
| Any key during boot | Skip the boot animation |

## Project layout

- `Pipboy3000.c` — application lifecycle and top-level rendering.
- `state.c` — state initialization, cleanup, XP, and damage state.
- `inventory.c` — CSV loading and inventory selection helpers.
- `resources.c` — fonts and static textures loaded once for reuse.
- `MAP/` — current map renderer and future offline-map integration point.
- `tests/` — focused regression tests for state, inventory, and navigation.
- `scripts/build.ps1` — dependency-free Windows build/test entry point.

## License

The source is licensed under the MIT License. See [LICENSE](LICENSE). Map data and generated country packs must retain the attribution and licensing required by their data provider.
