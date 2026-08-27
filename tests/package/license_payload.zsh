#!/usr/bin/env zsh
# Verify that install/CPack payloads carry every applicable license notice.
emulate -R zsh
setopt err_exit no_unset pipe_fail

source_root=${0:A:h:h:h}
build_dir=${ZPMOD_BUILD_DIR:-$source_root/build-cmake}
prefix=$(mktemp -d 2>/dev/null || mktemp -d -t zpmod-license-payload)
trap 'rm -rf -- "$prefix"' EXIT

fail_test() {
  print -ru2 -- "$*"
  exit 1
}

for source_notice in LICENSE NOTICE LICENSES/MIT.txt; do
  [[ -f "$source_root/$source_notice" ]] ||
    fail_test "missing source notice: $source_notice"
done

cmake --install "$build_dir" --prefix "$prefix" >/dev/null ||
  fail_test 'staged install failed'

install_data_dir=$(sed -n 's/^ZPMOD_DATA_INSTALL_DIR:INTERNAL=//p' \
  "$build_dir/CMakeCache.txt")
[[ -n $install_data_dir ]] || fail_test 'effective data install directory is unavailable'
if [[ $install_data_dir == /* ]]; then
  installed_data_root=$install_data_dir
else
  installed_data_root="$prefix/$install_data_dir"
fi

license_dir="$installed_data_root/licenses/zpmod"
for installed_notice in LICENSE NOTICE MIT.txt LicenseRef-zsh.txt; do
  [[ -s "$license_dir/$installed_notice" ]] ||
    fail_test "missing installed notice: share/licenses/zpmod/$installed_notice"
done
[[ -s "$installed_data_root/doc/zpmod/copyright" ]] ||
  fail_test 'missing Debian-style copyright notice'
grep -q 'Permission is hereby granted' "$installed_data_root/doc/zpmod/copyright" ||
  fail_test 'Debian-style copyright file omits the MIT terms'
grep -q 'The Z Shell is copyright' "$installed_data_root/doc/zpmod/copyright" ||
  fail_test 'Debian-style copyright file omits the Zsh terms'

grep -q 'SPDX-License-Identifier: MIT AND LicenseRef-zsh' \
  "$license_dir/NOTICE" ||
  fail_test 'NOTICE lacks the package license expression'
grep -q 'set(CPACK_RPM_PACKAGE_LICENSE "MIT AND LicenseRef-zsh")' \
  "$build_dir/CPackConfig.cmake" ||
  fail_test 'RPM metadata does not use the documented license expression'
grep -q '%license /usr/share/licenses/zpmod/NOTICE' \
  "$build_dir/CPackConfig.cmake" ||
  fail_test 'RPM metadata does not mark the installed notices as licenses'

# Inspect an actual generated archive rather than inferring CPack contents from
# install rules alone.
package_dir="$prefix/packages"
mkdir -p "$package_dir"
cpack --config "$build_dir/CPackConfig.cmake" -G TGZ -C Release \
  -B "$package_dir" >/dev/null || fail_test 'TGZ generation failed'
archives=( "$package_dir"/*.tar.gz(N) )
(( ${#archives} == 1 )) || fail_test 'expected exactly one TGZ package'
archive_listing=$(tar -tzf "$archives[1]")
for packaged_notice in LICENSE NOTICE MIT.txt LicenseRef-zsh.txt; do
  [[ $archive_listing == *"/share/licenses/zpmod/$packaged_notice"* ]] ||
    fail_test "TGZ omits $packaged_notice"
done
[[ $archive_listing == *'/share/doc/zpmod/copyright'* ]] ||
  fail_test 'TGZ omits the Debian-style copyright notice'

if command -v dpkg-deb >/dev/null 2>&1; then
  deb_dir="$prefix/deb-packages"
  mkdir -p "$deb_dir"
  cpack --config "$build_dir/CPackConfig.cmake" -G DEB -C Release \
    -B "$deb_dir" >/dev/null || fail_test 'DEB generation failed'
  debs=( "$deb_dir"/*.deb(N) )
  (( ${#debs} == 1 )) || fail_test 'expected exactly one DEB package'
  deb_listing=$(dpkg-deb --contents "$debs[1]")
  [[ $deb_listing == *'/usr/share/doc/zpmod/copyright'* ]] ||
    fail_test 'DEB omits /usr/share/doc/zpmod/copyright'
  [[ $deb_listing == *'/usr/share/licenses/zpmod/MIT.txt'* ]] ||
    fail_test 'DEB omits the MIT license text'
fi

if command -v rpmbuild >/dev/null 2>&1 && command -v rpm >/dev/null 2>&1; then
  rpm_dir="$prefix/rpm-packages"
  mkdir -p "$rpm_dir"
  cpack --config "$build_dir/CPackConfig.cmake" -G RPM -C Release \
    -B "$rpm_dir" >/dev/null || fail_test 'RPM generation failed'
  rpms=( "$rpm_dir"/*.rpm(N) )
  (( ${#rpms} == 1 )) || fail_test 'expected exactly one RPM package'
  rpm_licenses=$(rpm -qp --licensefiles "$rpms[1]")
  [[ $rpm_licenses == *'/usr/share/licenses/zpmod/NOTICE'* ]] ||
    fail_test 'RPM does not mark NOTICE as a license file'
  [[ $rpm_licenses == *'/usr/share/licenses/zpmod/LicenseRef-zsh.txt'* ]] ||
    fail_test 'RPM does not mark the Zsh license text as a license file'
fi

# A non-default GNUInstallDirs data directory must flow into RPM file flags.
custom_build="$prefix/custom-build"
cmake -S "$source_root" -B "$custom_build" \
  -DBUILD_TESTING=OFF \
  -DCMAKE_INSTALL_DATADIR=custom-share >/dev/null ||
  fail_test 'custom data-directory configure failed'
grep -q '%license /usr/custom-share/licenses/zpmod/NOTICE' \
  "$custom_build/CPackConfig.cmake" ||
  fail_test 'RPM license paths ignore CMAKE_INSTALL_DATADIR'
custom_prefix="$prefix/custom-prefix"
cmake --install "$custom_build" --prefix "$custom_prefix" \
  --component Licenses >/dev/null ||
  fail_test 'custom data-directory license install failed'
[[ -s "$custom_prefix/custom-share/licenses/zpmod/NOTICE" ]] ||
  fail_test 'custom data-directory install omits NOTICE'

absolute_data_dir="$prefix/absolute-share"
absolute_build="$prefix/absolute-build"
cmake -S "$source_root" -B "$absolute_build" \
  -DBUILD_TESTING=OFF \
  -DCMAKE_INSTALL_DATADIR="$absolute_data_dir" >/dev/null ||
  fail_test 'absolute data-directory configure failed'
grep -q "%license $absolute_data_dir/licenses/zpmod/NOTICE" \
  "$absolute_build/CPackConfig.cmake" ||
  fail_test 'RPM license paths corrupt an absolute CMAKE_INSTALL_DATADIR'
cmake --install "$absolute_build" --prefix "$prefix/ignored-prefix" \
  --component Licenses >/dev/null ||
  fail_test 'absolute data-directory license install failed'
[[ -s "$absolute_data_dir/licenses/zpmod/NOTICE" ]] ||
  fail_test 'absolute data-directory install omits NOTICE'

print -r -- 'package_license_payload OK'
