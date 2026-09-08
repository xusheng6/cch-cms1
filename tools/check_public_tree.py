#!/usr/bin/env python3
"""Fail closed on unexpected public paths; inspect staged blobs and Git history."""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ALLOWED_PATHS = frozenset('''
.gitignore
.github/workflows/ci.yml
README.md
LICENSE
NOTICE.md
VALIDATION.md
PUBLIC_EXPORT.json
docs/progress.svg
port/CMakeLists.txt
port/LICENSE
port/include/cch.h
port/src/book.c
port/src/bookdump.c
port/src/main.c
port/src/main_modern.c
port/src/movegen.c
port/src/original_constants.h
port/src/position.c
port/src/profiles.c
port/src/protocol.c
port/src/random_positions.c
port/src/search.c
port/tests/test_engine.c
port/tests/oracles/audit-100.jsonl
tools/compare_cms1_oracle.py
tools/test_cms1_oracle.py
tools/check_public_tree.py
tools/hooks/pre-commit
tools/hooks/pre-push
'''.split())


def check_content(path, data):
    if path not in ALLOWED_PATHS:
        raise ValueError(f'{path}: not in public allowlist')
    if len(data) > 2 * 1024 * 1024:
        raise ValueError(f'{path}: exceeds 2 MiB limit')
    if b'\0' in data:
        raise ValueError(f'{path}: binary/NUL payload')
    try:
        content = data.decode('utf-8')
    except UnicodeDecodeError:
        raise ValueError(f'{path}: not UTF-8 text') from None
    patterns = (
        r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----',
        r'\bgh[pousr]_[A-Za-z0-9]{30,}\b',
        r'\bgithub_pat_[A-Za-z0-9_]{40,}\b',
        r'\bAKIA[A-Z0-9]{16}\b',
        r'[A-Za-z0-9+/]{512,}={0,2}',
        r'(?:\\x[0-9A-Fa-f]{2}){128,}',
    )
    if any(re.search(pattern, content) for pattern in patterns):
        raise ValueError(f'{path}: possible credential or encoded payload')


def git(*args):
    return subprocess.check_output(['git', *args])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--history', action='store_true')
    parser.add_argument('--staged', action='store_true')
    parser.add_argument('--directory', type=Path,
                        help='Check a fresh export before Git initialization')
    args = parser.parse_args()
    seen = set()
    checked = 0
    if args.directory:
        paths = set()
        for file in args.directory.rglob('*'):
            if file.is_symlink():
                raise ValueError(f'{file.name}: symlinks are forbidden')
            if file.is_file():
                path = file.relative_to(args.directory).as_posix()
                check_content(path, file.read_bytes())
                paths.add(path)
        if paths != ALLOWED_PATHS:
            raise ValueError(f'missing public files: {sorted(ALLOWED_PATHS - paths)}')
        print(f'Public export check passed: {len(paths)} files')
        return

    def check_entry(mode, oid, path):
        nonlocal checked
        if path not in ALLOWED_PATHS:
            raise ValueError(f'{path}: not in public allowlist')
        if mode not in ('100644', '100755'):
            raise ValueError(f'{path}: only regular files are allowed')
        if (oid, path) not in seen:
            check_content(path, git('cat-file', 'blob', oid))
            seen.add((oid, path))
            checked += 1

    # Always check the index, even for the first commit without HEAD.
    for entry in git('ls-files', '--stage', '-z').split(b'\0'):
        if entry:
            metadata, path = entry.decode('utf-8').split('\t', 1)
            mode, oid, stage = metadata.split()
            if stage != '0':
                raise ValueError(f'{path}: unresolved merge entry')
            check_entry(mode, oid, path)
    commits = git('rev-list', '--all').decode().split() if args.history else []
    for commit in commits:
        for entry in git('ls-tree', '-rz', '--full-tree', commit).split(b'\0'):
            if entry:
                metadata, path = entry.decode('utf-8').split('\t', 1)
                mode, kind, oid = metadata.split()
                check_entry(mode, oid, path)
    print(f'Public content check passed: {checked} distinct file versions; '
          f'{len(commits)} historical commits')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, subprocess.CalledProcessError) as error:
        print(f'Public content check FAILED: {error}', file=sys.stderr)
        sys.exit(1)
