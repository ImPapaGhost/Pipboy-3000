#!/usr/bin/env python3
"""Convert numbered Pip-Boy image sequences into verified video assets."""

from __future__ import annotations

import argparse
import math
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Pattern

import cv2
import numpy as np


@dataclass(frozen=True)
class Sequence:
    name: str
    source: Path
    pattern: Pattern[str]
    output: Path
    frame_duration_ms: float
    size: tuple[int, int]


def numeric_pattern(extension: str) -> Pattern[str]:
    return re.compile(rf"^(\d+)\.{re.escape(extension)}$", re.IGNORECASE)


SEQUENCES = (
    Sequence("bootup", Path("BOOT/BOOTUP"), numeric_pattern("jpg"), Path("BOOT/bootup"), 80, (800, 480)),
    Sequence("bootboy", Path("BOOT/BootBoy"), numeric_pattern("jpg"), Path("BOOT/bootboy"), 120, (800, 480)),
    Sequence("vaultboy", Path("STAT/VaultBoy"), re.compile(r"^(\d{2})\.png$", re.IGNORECASE), Path("STAT/vaultboy"), 100, (160, 256)),
    Sequence("vaultboy-combat", Path("STAT/VaultBoy"), re.compile(r"^com(\d{2})\.png$", re.IGNORECASE), Path("STAT/vaultboy-combat"), 100, (160, 256)),
    Sequence("strength", Path("STAT/strength"), numeric_pattern("jpg"), Path("STAT/strength"), 175, (304, 176)),
    Sequence("perception", Path("STAT/perception"), numeric_pattern("jpg"), Path("STAT/perception"), 175, (304, 176)),
    Sequence("endurance", Path("STAT/endurance"), numeric_pattern("jpg"), Path("STAT/endurance"), 175, (304, 176)),
    Sequence("charisma", Path("STAT/charisma"), numeric_pattern("jpg"), Path("STAT/charisma"), 175, (304, 176)),
    Sequence("intelligence", Path("STAT/intelligence"), numeric_pattern("jpg"), Path("STAT/intelligence"), 175, (304, 176)),
    Sequence("agility", Path("STAT/agility"), numeric_pattern("jpg"), Path("STAT/agility"), 175, (304, 176)),
    Sequence("luck", Path("STAT/luck"), numeric_pattern("jpg"), Path("STAT/luck"), 175, (304, 176)),
    Sequence("radio", Path("RADIO"), numeric_pattern("jpg"), Path("RADIO/radio-waveform"), 40, (224, 224)),
)

FORMATS = {
    "mpg": ("PIM1", ".mpg"),
    "webm": ("VP90", ".webm"),
    "mp4": ("mp4v", ".mp4"),
}


def matching_frames(repo_root: Path, sequence: Sequence) -> list[Path]:
    source_directory = repo_root / sequence.source
    if not source_directory.is_dir():
        raise ValueError(
            f"Source frames are not present at {sequence.source}. "
            "Restore them from Git history or export a replacement sequence first."
        )

    indexed_frames: list[tuple[int, Path]] = []
    for path in source_directory.iterdir():
        match = sequence.pattern.fullmatch(path.name)
        if match:
            indexed_frames.append((int(match.group(1)), path))

    indexed_frames.sort(key=lambda entry: entry[0])
    if not indexed_frames:
        raise ValueError(f"No frames matched {sequence.pattern.pattern} in {sequence.source}")

    indices = [index for index, _ in indexed_frames]
    if len(indices) != len(set(indices)):
        raise ValueError(f"Duplicate frame indices in {sequence.source}: {indices}")

    return [path for _, path in indexed_frames]


def load_normalized_frame(path: Path, size: tuple[int, int]) -> np.ndarray:
    frame = cv2.imread(str(path), cv2.IMREAD_UNCHANGED)
    if frame is None:
        raise ValueError(f"Could not decode {path}")

    if frame.ndim == 2:
        frame = cv2.cvtColor(frame, cv2.COLOR_GRAY2BGR)
    elif frame.shape[2] == 4:
        color = frame[:, :, :3].astype(np.float32)
        alpha = frame[:, :, 3:4].astype(np.float32) / 255.0
        frame = np.clip(color * alpha, 0, 255).astype(np.uint8)

    if (frame.shape[1], frame.shape[0]) != size:
        frame = cv2.resize(frame, size, interpolation=cv2.INTER_AREA)
    return frame


def output_frame_sources(
    frame_paths: list[Path],
    frame_duration_ms: float,
    output_format: str,
) -> tuple[float, list[Path]]:
    if output_format != "mpg":
        return 1000.0 / frame_duration_ms, frame_paths

    # MPEG-1 supports a fixed set of frame rates. Encode at 25 FPS and repeat
    # source frames as needed to preserve their original display duration.
    output_fps = 25.0
    duration_seconds = len(frame_paths) * frame_duration_ms / 1000.0
    output_count = max(1, round(duration_seconds * output_fps))
    sources: list[Path] = []
    for output_index in range(output_count):
        time_ms = output_index * 1000.0 / output_fps
        source_index = min(int(time_ms / frame_duration_ms), len(frame_paths) - 1)
        sources.append(frame_paths[source_index])
    return output_fps, sources


def verify_video(
    video_path: Path,
    expected_sources: list[Path],
    expected_fps: float,
    expected_size: tuple[int, int],
) -> tuple[int, float, float, str]:
    capture = cv2.VideoCapture(str(video_path))
    if not capture.isOpened():
        raise RuntimeError(f"Could not reopen generated video {video_path}")

    reported_fps = capture.get(cv2.CAP_PROP_FPS)
    width = int(capture.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(capture.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fourcc = int(capture.get(cv2.CAP_PROP_FOURCC))
    codec = "".join(chr((fourcc >> (8 * index)) & 0xFF) for index in range(4)).rstrip("\x00")

    decoded_count = 0
    minimum_psnr = math.inf
    while True:
        success, decoded = capture.read()
        if not success:
            break
        if decoded_count >= len(expected_sources):
            capture.release()
            raise RuntimeError(f"{video_path} decoded more frames than expected")
        expected = load_normalized_frame(expected_sources[decoded_count], expected_size)
        minimum_psnr = min(minimum_psnr, cv2.PSNR(expected, decoded))
        decoded_count += 1
    capture.release()

    if decoded_count != len(expected_sources):
        raise RuntimeError(f"{video_path} decoded {decoded_count} frames; expected {len(expected_sources)}")
    if not math.isclose(reported_fps, expected_fps, rel_tol=0.001, abs_tol=0.001):
        raise RuntimeError(f"{video_path} reports {reported_fps:g} FPS; expected {expected_fps:g}")
    if (width, height) != expected_size:
        raise RuntimeError(f"{video_path} is {width}x{height}; expected {expected_size[0]}x{expected_size[1]}")

    return decoded_count, reported_fps, minimum_psnr, codec


def convert_sequence(repo_root: Path, sequence: Sequence, output_format: str) -> None:
    codec, suffix = FORMATS[output_format]
    frame_paths = matching_frames(repo_root, sequence)
    output_fps, expected_sources = output_frame_sources(
        frame_paths,
        sequence.frame_duration_ms,
        output_format,
    )

    output_path = (repo_root / sequence.output).with_suffix(suffix)
    temporary_path = output_path.with_name(f"{output_path.stem}.tmp{output_path.suffix}")
    writer = cv2.VideoWriter(
        str(temporary_path),
        cv2.VideoWriter_fourcc(*codec),
        output_fps,
        sequence.size,
    )
    if not writer.isOpened():
        raise RuntimeError(f"OpenCV could not initialize the {codec} encoder")

    try:
        for frame_path in expected_sources:
            writer.write(load_normalized_frame(frame_path, sequence.size))
    finally:
        writer.release()

    try:
        decoded_count, decoded_fps, minimum_psnr, decoded_codec = verify_video(
            temporary_path,
            expected_sources,
            output_fps,
            sequence.size,
        )
    except Exception:
        temporary_path.unlink(missing_ok=True)
        raise

    temporary_path.replace(output_path)
    duration = decoded_count / decoded_fps
    print(
        f"{sequence.name:18} -> {output_path.relative_to(repo_root)}: "
        f"{decoded_count} frames, {decoded_fps:.3f} FPS, {duration:.3f} s, "
        f"{sequence.size[0]}x{sequence.size[1]}, {decoded_codec}, "
        f"min PSNR {minimum_psnr:.2f} dB, {output_path.stat().st_size / 1024:.1f} KiB"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--format",
        choices=FORMATS,
        default="mpg",
        help="Output container/codec (default: MPEG-1 program stream for the embedded decoder)",
    )
    parser.add_argument(
        "--only",
        choices=[sequence.name for sequence in SEQUENCES],
        action="append",
        help="Convert only the named sequence; repeat to select more than one",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    selected = [sequence for sequence in SEQUENCES if not args.only or sequence.name in args.only]
    for sequence in selected:
        convert_sequence(repo_root, sequence, args.format)


if __name__ == "__main__":
    main()
