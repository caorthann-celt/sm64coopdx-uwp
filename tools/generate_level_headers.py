#!/usr/bin/env python3
import re
import sys
from pathlib import Path


DEFINE_LEVEL_RE = re.compile(
    r'^\s*DEFINE_LEVEL\(\s*"[^"]*"\s*,\s*[^,]+\s*,\s*[^,]+\s*,\s*([A-Za-z0-9_]+)\s*,'
)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_level_headers.py <level_defines.h>", file=sys.stderr)
        return 1

    level_defines = Path(sys.argv[1])
    seen = set()

    with level_defines.open("r", encoding="utf-8") as f:
        for line in f:
            match = DEFINE_LEVEL_RE.match(line)
            if not match:
                continue

            folder = match.group(1)
            if folder in seen:
                continue
            seen.add(folder)
            print(f'#include "levels/{folder}/header.h"')

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
