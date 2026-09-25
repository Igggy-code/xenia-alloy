#!/usr/bin/env python3
"""Run native CPU/base tests and validate every embedded shader on the real GPU."""
import argparse
from pathlib import Path
import re
import subprocess


def run(*args, timeout=120):
    subprocess.run([str(arg) for arg in args], check=True, timeout=timeout)


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=root / 'build-macos-arm64')
    parser.add_argument('--metal-only', action='store_true')
    args = parser.parse_args()
    build = args.build_dir.resolve()
    if not args.metal_only:
        for suite in ('base', 'cpu'):
            run(build / 'bin/macOS' / f'xenia-{suite}-tests')

    scratch = build / 'metal-validation'
    scratch.mkdir(parents=True, exist_ok=True)
    sources = []
    for header in sorted((build / 'generated/xenia').glob('*/shaders/bytecode/metal/*.h')):
        match = re.search(r'R"XE_METAL_SOURCE\((.*)\)XE_METAL_SOURCE"',
                          header.read_text(), re.S)
        if not match:
            raise RuntimeError(f'Missing embedded source: {header}')
        source = scratch / (header.parent.parents[2].name + '_' + header.stem + '.metal')
        source.write_text(match[1])
        sources.append(source)
    if not sources:
        raise RuntimeError('No generated Metal shaders. Build the application first.')
    compiler = subprocess.check_output(
        ['/usr/bin/arch', '-arm64', '/usr/bin/xcrun', '--find', 'clang++'],
        text=True).strip()
    sdk = subprocess.check_output(
        ['/usr/bin/arch', '-arm64', '/usr/bin/xcrun', '--show-sdk-path'],
        text=True).strip()
    probe = scratch / 'metal_shader_probe'
    run(compiler, '-std=c++17', '-arch', 'arm64', '-isysroot', sdk,
        '-mmacosx-version-min=15.0',
        '-framework', 'Foundation', '-framework', 'Metal',
        root / 'tools/testing/metal_shader_probe.mm', '-o', probe)
    run(probe, *sources, timeout=300)


if __name__ == '__main__':
    main()
