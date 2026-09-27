#!/usr/bin/env bash
# Run only in the disposable, matching distribution qualification container.
set -euo pipefail
: "${PACKAGE_FORMAT:?}"
mkdir -p qualification-results/packages
cpack --config build-cmake/CPackConfig.cmake -G "${PACKAGE_FORMAT}" -C Release \
  -B qualification-results/packages
case ${PACKAGE_FORMAT} in
DEB)
  packages=(qualification-results/packages/*.deb)
  [[ ${#packages[@]} == 1 && -f ${packages[0]} ]]
  dpkg -i "${packages[0]}"
  remove_package() { dpkg -r zpmod; }
  ;;
RPM)
  packages=(qualification-results/packages/*.rpm)
  [[ ${#packages[@]} == 1 && -f ${packages[0]} ]]
  rpm -Uvh "${packages[0]}"
  remove_package() { rpm -e zpmod; }
  ;;
*) exit 1 ;;
esac
trap remove_package EXIT
runtime=$(command -v zsh)
python3 tests/package/installed_profile.py --zsh "${runtime}" \
  --prefix /usr --report qualification-results/package-install.json
remove_package
trap - EXIT
[[ ! -e /usr/lib/zsh/site-modules/zpmod.so ]]
python3 - "${packages[0]}" <<'PY'
import hashlib
import json
import platform
import subprocess
import sys
from pathlib import Path
artifact = Path(sys.argv[1])
Path('qualification-results/package-provenance.json').write_text(json.dumps({
    'candidate': subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
    'headers': subprocess.check_output(['git', '-C', 'vendor/zsh', 'rev-parse', 'HEAD'], text=True).strip(),
    'zsh': subprocess.check_output(['zsh', '--version'], text=True).strip(),
    'architecture': platform.machine(), 'distribution': Path('/etc/os-release').read_text(),
    'package': artifact.name, 'sha256': hashlib.sha256(artifact.read_bytes()).hexdigest(),
    'install_and_remove': 'passed'
}, indent=2) + '\n')
PY
