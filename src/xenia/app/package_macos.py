#!/usr/bin/env python3
"""Bundle non-system dylibs and ad-hoc sign the native development app."""
import argparse
from pathlib import Path
import shutil
import subprocess


# CMake/Python may run under Rosetta. Apple's command wrappers must use the
# native architecture so they load the matching libxcrun on Apple Silicon.
_native_tools = []
if subprocess.run(['/usr/bin/arch', '-arm64', '/usr/bin/true'],
                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0:
    _native_tools = ['/usr/bin/arch', '-arm64']


def run(*args):
    return subprocess.check_output([*_native_tools, *args], text=True)


def dependencies(binary):
    lines = run('/usr/bin/otool', '-L', str(binary)).splitlines()
    return {line.strip().split(' (compatibility version', 1)[0]
            for line in lines if line.startswith('\t')}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, required=True)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--entitlements', type=Path, required=True)
    parser.add_argument('--library', action='append', type=Path, default=[])
    parser.add_argument('--license', action='append', nargs=2, default=[],
                        metavar=('SOURCE', 'BUNDLE_NAME'))
    args = parser.parse_args()
    frameworks = args.app / 'Contents' / 'Frameworks'
    frameworks.mkdir(parents=True, exist_ok=True)
    licenses = args.app / 'Contents' / 'Resources' / 'Licenses'
    licenses.mkdir(parents=True, exist_ok=True)
    for source, bundle_name in args.license:
        destination = licenses / bundle_name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    libraries = {library.name: library for library in args.library}
    copied = {}
    queue = [args.executable]
    while queue:
        binary = queue.pop()
        for dep in sorted(dependencies(binary)):
            if dep.startswith(('/System/', '/usr/lib/')):
                continue
            name = Path(dep).name
            source = libraries.get(name)
            if source is None and dep.startswith('/'):
                source = Path(dep)
            if source is None:
                raise RuntimeError(f'Unresolved dependency {dep} in {binary}')
            destination = frameworks / name
            if name not in copied:
                shutil.copy2(source, destination)
                destination.chmod(destination.stat().st_mode | 0o200)
                copied[name] = destination
                run('/usr/bin/install_name_tool', '-id', f'@rpath/{name}', str(destination))
                queue.append(destination)
            replacement = f'@rpath/{name}'
            if dep != replacement:
                run('/usr/bin/install_name_tool', '-change', dep, replacement, str(binary))
    for library in copied.values():
        run('/usr/bin/codesign', '--force', '--sign', '-', str(library))
    run('/usr/bin/codesign', '--force', '--sign', '-', '--entitlements',
        str(args.entitlements), str(args.app))
    run('/usr/bin/codesign', '--verify', '--deep', '--strict', str(args.app))


if __name__ == '__main__':
    main()
