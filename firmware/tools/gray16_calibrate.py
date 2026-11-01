#!/usr/bin/env python3
"""Calculate 16-channel grayscale calibration constants from UART logs."""

from __future__ import annotations

import argparse
import re
import statistics
from pathlib import Path


CHANNEL_COUNT = 16
ADC_MIN = 0
ADC_MAX = 4095
MIN_CALIBRATION_SPAN = 100

CSV_PATTERN = re.compile(
    r"(?<!\d)(\d{1,4}(?:\s*,\s*\d{1,4}){15})(?!\s*,\s*\d)"
)


def load_samples(path: Path) -> list[list[int]]:
    samples: list[list[int]] = []

    for line_number, line in enumerate(
        path.read_text(encoding="utf-8", errors="ignore").splitlines(),
        start=1,
    ):
        match = CSV_PATTERN.search(line)
        if match is None:
            continue

        values = [
            int(value.strip())
            for value in match.group(1).split(",")
        ]

        if len(values) != CHANNEL_COUNT:
            continue

        if any(
            value < ADC_MIN or value > ADC_MAX
            for value in values
        ):
            raise ValueError(
                f"{path}:{line_number}: ADC value outside 0..4095"
            )

        samples.append(values)

    if not samples:
        raise ValueError(
            f"{path}: no valid lines containing {CHANNEL_COUNT} ADC values"
        )

    return samples


def trimmed_mean(values: list[int]) -> int:
    ordered = sorted(values)

    if len(ordered) >= 10:
        trim_count = max(1, len(ordered) // 10)
        ordered = ordered[trim_count:-trim_count]

    return int(round(statistics.fmean(ordered)))


def channel_means(samples: list[list[int]]) -> list[int]:
    return [
        trimmed_mean([sample[channel] for sample in samples])
        for channel in range(CHANNEL_COUNT)
    ]


def format_c_array(name: str, values: list[int]) -> str:
    lines = [
        "static const uint16_t",
        f"{name}[LQ_GRAY_CHANNEL_COUNT] = {{",
    ]

    for start in range(0, CHANNEL_COUNT, 4):
        group = values[start:start + 4]
        suffix = "," if start + 4 < CHANNEL_COUNT else ""
        lines.append(
            "    " + ", ".join(f"{value}U" for value in group) + suffix
        )

    lines.append("};")
    return "\n".join(lines)


def print_diagnostics(
    white_samples: list[list[int]],
    black_samples: list[list[int]],
    white_means: list[int],
    black_means: list[int],
) -> bool:
    all_valid = True

    print("\nChannel diagnostics:")
    print("CH  WHITE  BLACK  SPAN  WHITE_RANGE  BLACK_RANGE  STATUS")

    for channel in range(CHANNEL_COUNT):
        white_values = [row[channel] for row in white_samples]
        black_values = [row[channel] for row in black_samples]
        span = abs(black_means[channel] - white_means[channel])
        status = "OK"

        if span < MIN_CALIBRATION_SPAN:
            status = "DISABLE/CHECK"
            all_valid = False

        print(
            f"D{channel + 1:<2} "
            f"{white_means[channel]:>6} "
            f"{black_means[channel]:>6} "
            f"{span:>5} "
            f"{max(white_values) - min(white_values):>11} "
            f"{max(black_values) - min(black_values):>11} "
            f"{status}"
        )

    return all_valid


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Calculate D1-D16 white/black calibration constants "
            "from UART CSV logs."
        )
    )
    parser.add_argument("white_log", type=Path)
    parser.add_argument("black_log", type=Path)
    args = parser.parse_args()

    white_samples = load_samples(args.white_log)
    black_samples = load_samples(args.black_log)
    white_means = channel_means(white_samples)
    black_means = channel_means(black_samples)

    print(
        f"Loaded {len(white_samples)} white samples and "
        f"{len(black_samples)} black samples.\n"
    )
    print(format_c_array("s_lq_gray_white_adc", white_means))
    print()
    print(format_c_array("s_lq_gray_black_adc", black_means))

    all_valid = print_diagnostics(
        white_samples,
        black_samples,
        white_means,
        black_means,
    )

    if not all_valid:
        print(
            "\nAt least one channel has a calibration span below "
            f"{MIN_CALIBRATION_SPAN}. Check its wiring/height or disable it."
        )
        return 2

    print("\nAll 16 calibration spans are valid.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
