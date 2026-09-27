#!/usr/bin/env python3
"""Validate an installed module in a clean process, without a build-tree path."""
import argparse
import hashlib
import json
import os
import subprocess
import tempfile
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--zsh', required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    modules = [module for lib in ('lib', 'lib64')
               for module in (prefix / lib / 'zsh/site-modules').glob('zpmod.*')]
    if len(modules) != 1 or not modules[0].is_file():
        raise SystemExit('expected exactly one installed module')
    for relative in ('share/zsh/site-functions/_zpmod', 'share/licenses/zpmod/LICENSE',
                     'share/licenses/zpmod/NOTICE', 'share/licenses/zpmod/MIT.txt',
                     'share/licenses/zpmod/LicenseRef-zsh.txt', 'share/doc/zpmod/copyright'):
        if not (prefix / relative).is_file():
            raise SystemExit(f'missing installed payload: {relative}')
    with tempfile.TemporaryDirectory(prefix='zpmod-installed-') as home:
        fixture = Path(home) / 'fixture.zsh'
        fixture.write_text(':\n')
        # Zsh test-fixture under standalone native state, floor 5.8.1.
        script = '''
          emulate -R zsh
          module_path=( "$3" $module_path )
          zmodload zpmod || exit 90
          fpath=( "$1/share/zsh/site-functions" $fpath )
          autoload -Uz _zpmod
          [[ $(whence -w _zpmod) == *function* ]] || exit 91
          source "$2" || exit 92
          zpmod source-study --json
        '''
        result = subprocess.run([args.zsh, '-f', '-c', script, '--', str(prefix),
                                 str(fixture), str(modules[0].parent)],
                                env={'PATH': os.environ['PATH'], 'HOME': home,
                                     'ZDOTDIR': home, 'LC_ALL': 'C'},
                                cwd=home, capture_output=True, check=True, timeout=30)
        report = json.loads(result.stdout)
        assert report['schema_version'] == 1
        assert report['status'] == 'complete'
        assert report['inclusive'] is True
        assert report['unit'] == 'nanoseconds'
        assert report['execution_mode'] == 'instrumented-auto-compile'
        event, = report['events']
        assert bytes.fromhex(event['path_hex']) == os.fsencode(fixture)
        assert event['source_status'] == event['exit_status'] == 0
        assert type(event['duration_ns']) is int and event['duration_ns'] >= 0
        assert event['id'] == event['depth'] == 1
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({
        'schema': 'z-shell/zpmod-installed-qualification/v1',
        'module_path': str(modules[0].relative_to(prefix)),
        'module_sha256': hashlib.sha256(modules[0].read_bytes()).hexdigest(),
        'source_study': 'passed', 'payload': 'passed',
    }, indent=2) + '\n')
    print('Installed payload and source-study JSON passed')


if __name__ == '__main__':
    main()
