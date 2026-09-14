#!/usr/bin/env python3
"""Keep the first k IDs of each TexMex .ivecs row, updating its width header."""

import argparse
from pathlib import Path
import struct


def truncate_ivecs(source: Path, destination: Path, k: int) -> int:
    if k <= 0:
        raise ValueError("k must be positive")
    if source.resolve() == destination.resolve():
        raise ValueError("input and output must be different files")

    data = source.read_bytes()
    output = bytearray()
    offset = 0
    rows = 0
    while offset < len(data):
        if len(data) - offset < 4:
            raise ValueError(f"row {rows + 1}: incomplete width header")
        width = struct.unpack_from("<i", data, offset)[0]
        offset += 4
        if width <= 0 or width > (len(data) - offset) // 4:
            raise ValueError(f"row {rows + 1}: invalid width or truncated IDs")
        if k > width:
            raise ValueError(f"row {rows + 1}: requested k={k}, but only {width} IDs exist")
        output.extend(struct.pack("<i", k))
        output.extend(data[offset : offset + k * 4])
        offset += width * 4
        rows += 1

    if rows == 0:
        raise ValueError("input contains no ground-truth rows")
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(output)
    return rows


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="source ground truth (.ivecs)")
    parser.add_argument("output", type=Path, help="output ground truth (.ivecs)")
    parser.add_argument("-k", type=int, required=True, help="number of IDs to retain per query")
    args = parser.parse_args()
    try:
        rows = truncate_ivecs(args.input, args.output, args.k)
    except (OSError, ValueError) as error:
        parser.exit(1, f"error: {error}\n")
    print(f"Wrote {rows} rows with {args.k} neighbor IDs each to {args.output}")


if __name__ == "__main__":
    main()
