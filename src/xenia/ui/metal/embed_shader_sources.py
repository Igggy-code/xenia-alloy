#!/usr/bin/env python3
"""Embed current XeSL/MSL sources for compilation by the installed Metal driver.

Only project-local quoted includes are expanded. Metal's system headers and the
language-specific conditional preprocessor blocks remain for the runtime compiler.
Generated files are reproducible and are rewritten only when contents change.
"""
import argparse
from pathlib import Path
import re


def expand(path, root, stack=()):
    path = path.resolve()
    if path in stack:
        raise ValueError(f"Recursive shader include: {path}")
    text = path.read_text()
    def include(match):
        child = (path.parent / match[1]).resolve()
        if not child.is_relative_to(root):
            raise ValueError(f"Shader include outside project: {child}")
        return expand(child, root, (*stack, path))
    return re.sub(r'^\s*#\s*include\s+"([^"]+)"[^\n]*$', include, text, flags=re.M)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    used = set()
    for sub in ('src/xenia/gpu/metal', 'src/xenia/ui/metal'):
        for file in (root / sub).iterdir():
            if file.suffix in ('.cc', '.mm', '.h'):
                used.update(re.findall(r'"(xenia/(?:gpu|ui)/shaders/bytecode/metal/[^\"]+\.h)"', file.read_text()))
    for include in sorted(used):
        prefix, filename = include.split('/bytecode/metal/')
        identifier = filename[:-2]
        stem, stage = identifier.rsplit('_', 1)
        source = root / 'src' / prefix / f'{stem}.{stage}.metal'
        if not source.exists():
            source = source.with_suffix('.xesl')
        if not source.exists():
            raise FileNotFoundError(f"No current source for {include}: {source}")
        msl = '#define SHADING_LANGUAGE_MSL_XE 1\n' + expand(source, root)
        delimiter = 'XE_METAL_SOURCE'
        if ')' + delimiter + '"' in msl:
            raise ValueError(f"Raw string delimiter collision in {source}")
        generated = ('// Generated from current shader sources; compiled by Metal at runtime.\n'
                     '#pragma once\n#include <cstdint>\n'
                     f'const uint8_t {identifier}_metallib[] = R"{delimiter}({msl}){delimiter}";\n')
        output = args.output / include
        output.parent.mkdir(parents=True, exist_ok=True)
        if not output.exists() or output.read_text() != generated:
            output.write_text(generated)


if __name__ == '__main__':
    main()
