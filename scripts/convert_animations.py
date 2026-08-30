#!/usr/bin/env python3
"""Convert the boot JPEG sequences into constant-frame-rate video files."""

from __future__ import annotations

import argparse
import math
from dataclasses import dataclass
from pathlib import Path

import cv2


@dataclass(frozen=True)
class Sequence:
    source: Path
    output_name: str
    fps: float
    size: tuple[int, int] = (800, 480)


SEQUENCES = (
    Sequence(Path("BOOT/BOOTUP"), "bootup", 12.5),       # 80 ms per frame
    Sequence(Path("BOOT/BootBoy"), "bootboy", 25.0 / 3), # 120 ms per frame
)

FORMATS = {
    "webm": ("VP90", ".webm"),
    "mp4": ("mp4v", ".mp4"),
}


def numbered_frames(directory: Path) -> list[Path]:
    frames = [path for path in directory.glob("*.jpg") if path.stem.isdigit()]
    frames.sort(key=lambda path: int(path.stem))

    expected = list(range(len(frames)))
    actual = [int(path.stem) for path in frames]
    if not frames or actual != expected:
        raise ValueError(
            f"{directory} must contain a contiguous zero-based JPEG sequence; "
            f"found indices {actual[:3]}...{actual[-3:] if actual else []}"
        )
    return frames


def convert_sequence(repo_root: Path, sequence: Sequence, output_format: str) -> None:
    codec, suffix = FORMATS[output_format]
    source_directory = repo_root / sequence.source
    frame_paths = numbered_frames(source_directory)

    first_frame = cv2.imread(str(frame_paths[0]), cv2.IMREAD_COLOR)
    if first_frame is None:
        raise ValueError(f"Could not decode {frame_paths[0]}")

    width, height = sequence.size
    output_path = repo_root / "BOOT" / f"{sequence.output_name}{suffix}"
    temporary_path = output_path.with_name(f"{output_path.stem}.tmp{output_path.suffix}")
    writer = cv2.VideoWriter(
        str(temporary_path),
        cv2.VideoWriter_fourcc(*codec),
        sequence.fps,
        (width, height),
    )
    if not writer.isOpened():
        raise RuntimeError(f"OpenCV could not initialize the {codec} encoder")

    try:
        for frame_path in frame_paths:
            frame = cv2.imread(str(frame_path), cv2.IMREAD_COLOR)
            if frame is None:
                raise ValueError(f"Could not decode {frame_path}")
            if frame.shape[:2] != (height, width):
                # SDL_RenderCopy previously stretched every source frame to the
                # 800x480 window, so normalize the two odd-sized JPEGs the same way.
                frame = cv2.resize(frame, (width, height), interpolation=cv2.INTER_AREA)
            writer.write(frame)
    finally:
        writer.release()

    capture = cv2.VideoCapture(str(temporary_path))
    if not capture.isOpened():
        temporary_path.unlink(missing_ok=True)
        raise RuntimeError(f"Could not reopen generated video {temporary_path}")

    decoded_frames = int(capture.get(cv2.CAP_PROP_FRAME_COUNT))
    decoded_fps = capture.get(cv2.CAP_PROP_FPS)
    decoded_width = int(capture.get(cv2.CAP_PROP_FRAME_WIDTH))
    decoded_height = int(capture.get(cv2.CAP_PROP_FRAME_HEIGHT))
    capture.release()

    if decoded_frames != len(frame_paths):
        temporary_path.unlink(missing_ok=True)
        raise RuntimeError(
            f"Generated video contains {decoded_frames} frames; expected {len(frame_paths)}"
        )
    if not math.isclose(decoded_fps, sequence.fps, rel_tol=0.001, abs_tol=0.001):
        temporary_path.unlink(missing_ok=True)
        raise RuntimeError(f"Generated video reports {decoded_fps:g} FPS; expected {sequence.fps:g}")
    if (decoded_width, decoded_height) != (width, height):
        temporary_path.unlink(missing_ok=True)
        raise RuntimeError(
            f"Generated video is {decoded_width}x{decoded_height}; expected {width}x{height}"
        )

    temporary_path.replace(output_path)
    duration = decoded_frames / decoded_fps
    print(
        f"{output_path.relative_to(repo_root)}: {decoded_frames} frames, "
        f"{decoded_fps:.3f} FPS, {duration:.3f} s, {width}x{height}, "
        f"{output_path.stat().st_size / 1024:.1f} KiB"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--format",
        choices=FORMATS,
        default="webm",
        help="Output container/codec (default: webm with VP9)",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    for sequence in SEQUENCES:
        convert_sequence(repo_root, sequence, args.format)


if __name__ == "__main__":
    main()
