#!/usr/bin/env python3
"""Stage an install and compile a minimal external consumer."""

from __future__ import annotations

import os
import pathlib
import shutil
import subprocess
import sys
import tempfile


def find_single_path(root: pathlib.Path, pattern: str) -> pathlib.Path:
    matches = sorted(root.rglob(pattern))
    if len(matches) != 1:
        print(
            f"FAIL: expected exactly one match for {pattern!r} under {root}, "
            f"found {len(matches)}",
            file=sys.stderr,
        )
        for match in matches:
            print(f"  {match}", file=sys.stderr)
        raise SystemExit(1)
    return matches[0]


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: run_install_smoke.py <source_root> <build_root>",
              file=sys.stderr)
        return 2

    source_root = pathlib.Path(sys.argv[1]).resolve()
    build_root = pathlib.Path(sys.argv[2]).resolve()
    compiler = os.environ.get("CC", "cc")

    meson = shutil.which("meson")
    if meson is None:
        print("FAIL: meson not found in PATH", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="arena-install-smoke-") as temp_dir:
        temp_path = pathlib.Path(temp_dir)
        destdir = temp_path / "destdir"
        exe_path = temp_path / "install_smoke_consumer"
        consumer = source_root / "tests" / "install_smoke_consumer.c"

        subprocess.run(
            [meson, "install", "-C", str(build_root), "--destdir",
             str(destdir)],
            check=True,
        )

        include_dir = find_single_path(destdir, "arena.h").parent
        lib_path = find_single_path(destdir, "libarena.a")

        subprocess.run(
            [
                compiler,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                str(consumer),
                "-I",
                str(include_dir),
                str(lib_path),
                "-o",
                str(exe_path),
            ],
            check=True,
        )

        subprocess.run([str(exe_path)], check=True)

    print("PASS: arena_install_smoke")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
