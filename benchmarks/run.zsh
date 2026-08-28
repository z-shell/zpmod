#!/usr/bin/env zsh
# SPDX-License-Identifier: MIT

emulate -R zsh
setopt pipe_fail extended_glob
zmodload zsh/datetime || exit 1
zmodload zsh/mathfunc || exit 1

usage() {
  print -r -- "Usage: benchmarks/run.zsh --module-dir DIR --output-dir DIR [options]"
  print -r -- ""
  print -r -- "Options:"
  print -r -- "  --source-revision REF   Commit or release identifier (default: unknown)"
  print -r -- "  --environment LABEL    Public environment label"
  print -r -- "  --cpu LABEL            CPU label override"
  print -r -- "  --compiler LABEL       Module compiler label override"
  print -r -- "  --module-origin LABEL  Source build or release artifact name"
  print -r -- "  --origin-sha256 HASH   Release artifact SHA-256, when applicable"
  print -r -- "  --scripts COUNT        Generated scripts per workload (default: 40)"
  print -r -- "  --functions COUNT      Functions per generated script (default: 30)"
  print -r -- "  --warmups COUNT        Untimed rounds per case (default: 5)"
  print -r -- "  --runs COUNT           Timed rounds per case (default: 30)"
  print -r -- "  --help                 Show this help"
}

fail() {
  print -ru2 -- "benchmark: $*"
  exit 1
}

writef() {
  local output_descriptor=$1
  shift
  printf "$@" >&$output_descriptor
}

require_positive_integer() {
  local option_name=$1
  local option_value=$2
  [[ $option_value == <1-> ]] || fail "$option_name must be a positive integer"
}

require_metadata_scalar() {
  local option_name=$1
  local option_value=$2
  [[ $option_value != *$'\t'* && $option_value != *$'\n'* && $option_value != *$'\r'* && $option_value != *'`'* ]] ||
    fail "$option_name cannot contain tabs, line breaks, or backticks"
}

json_escape() {
  local value=$1
  value=${value//\\/\\\\}
  value=${value//\"/\\\"}
  value=${value//$'\n'/\\n}
  value=${value//$'\r'/\\r}
  value=${value//$'\t'/\\t}
  print -rn -- "$value"
}

module_sha256() {
  local module_file=$1
  local digest_output

  if (( $+commands[sha256sum] )); then
    digest_output=$(sha256sum -- "$module_file") || return
    print -r -- "${digest_output%% *}"
    return
  fi
  if (( $+commands[shasum] )); then
    digest_output=$(shasum -a 256 -- "$module_file") || return
    print -r -- "${digest_output%% *}"
    return
  fi
  return 1
}

typeset module_dir=""
typeset output_dir=""
typeset source_revision="unknown"
typeset environment_label=""
typeset cpu_label=""
typeset compiler_override=""
typeset module_origin="source build"
typeset origin_sha256="not applicable"
typeset script_count=40
typeset function_count=30
typeset warmup_count=5
typeset run_count=30

while (( $# )); do
  case $1 in
    --module-dir)
      (( $# >= 2 )) || fail "--module-dir requires a value"
      module_dir=$2
      shift 2
      ;;
    --output-dir)
      (( $# >= 2 )) || fail "--output-dir requires a value"
      output_dir=$2
      shift 2
      ;;
    --source-revision)
      (( $# >= 2 )) || fail "--source-revision requires a value"
      source_revision=$2
      shift 2
      ;;
    --environment)
      (( $# >= 2 )) || fail "--environment requires a value"
      environment_label=$2
      shift 2
      ;;
    --cpu)
      (( $# >= 2 )) || fail "--cpu requires a value"
      cpu_label=$2
      shift 2
      ;;
    --compiler)
      (( $# >= 2 )) || fail "--compiler requires a value"
      compiler_override=$2
      shift 2
      ;;
    --module-origin)
      (( $# >= 2 )) || fail "--module-origin requires a value"
      module_origin=$2
      shift 2
      ;;
    --origin-sha256)
      (( $# >= 2 )) || fail "--origin-sha256 requires a value"
      origin_sha256=$2
      shift 2
      ;;
    --scripts)
      (( $# >= 2 )) || fail "--scripts requires a value"
      script_count=$2
      shift 2
      ;;
    --functions)
      (( $# >= 2 )) || fail "--functions requires a value"
      function_count=$2
      shift 2
      ;;
    --warmups)
      (( $# >= 2 )) || fail "--warmups requires a value"
      warmup_count=$2
      shift 2
      ;;
    --runs)
      (( $# >= 2 )) || fail "--runs requires a value"
      run_count=$2
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      fail "unknown option: $1"
      ;;
  esac
done

[[ -n $module_dir ]] || fail "--module-dir is required"
[[ -d $module_dir ]] || fail "module directory does not exist: $module_dir"
[[ -n $output_dir ]] || fail "--output-dir is required"
require_positive_integer --scripts "$script_count"
require_positive_integer --functions "$function_count"
require_positive_integer --warmups "$warmup_count"
require_positive_integer --runs "$run_count"
require_metadata_scalar --source-revision "$source_revision"
require_metadata_scalar --environment "$environment_label"
require_metadata_scalar --cpu "$cpu_label"
require_metadata_scalar --compiler "$compiler_override"
require_metadata_scalar --module-origin "$module_origin"
require_metadata_scalar --origin-sha256 "$origin_sha256"
if [[ $origin_sha256 != "not applicable" ]]; then
  [[ ${#origin_sha256} == 64 && $origin_sha256 == [[:xdigit:]]## ]] || fail "--origin-sha256 must be a 64-character hexadecimal digest"
fi

module_dir=${module_dir:A}
mkdir -p -- "$output_dir" || fail "could not create output directory: $output_dir"
output_dir=${output_dir:A}

typeset -a module_files=()
typeset extension
for extension in so bundle dylib dll; do
  [[ -f "$module_dir/zpmod.$extension" ]] && module_files+=( "$module_dir/zpmod.$extension" )
done
(( ${#module_files} == 1 )) || fail "expected exactly one zpmod module in $module_dir"
typeset module_file=${module_files[1]}

typeset benchmark_tmp_parent=${TMPDIR:-/tmp}
[[ -d $benchmark_tmp_parent && -w $benchmark_tmp_parent ]] || fail "temporary directory is not writable: $benchmark_tmp_parent"
benchmark_tmp_parent=${benchmark_tmp_parent:A}
typeset benchmark_root
benchmark_root=$(mktemp -d "$benchmark_tmp_parent/zpmod-benchmark.XXXXXX") || fail "could not create temporary directory"

cleanup_benchmark() {
  if [[ -n ${benchmark_root:-} && -d $benchmark_root && $benchmark_root == "$benchmark_tmp_parent"/zpmod-benchmark.* ]]; then
    rm -rf -- "$benchmark_root"
  fi
}
TRAPEXIT() {
  cleanup_benchmark
}
TRAPINT() {
  cleanup_benchmark
  exit 130
}
TRAPQUIT() {
  cleanup_benchmark
  exit 131
}
TRAPTERM() {
  cleanup_benchmark
  exit 143
}

typeset isolated_home="$benchmark_root/home"
typeset isolated_zdotdir="$benchmark_root/zdotdir"
mkdir -p -- "$isolated_home" "$isolated_zdotdir" || fail "could not create isolated shell directories"

generate_workload() {
  builtin emulate -L zsh
  local workload_dir=$1
  local scripts_dir="$workload_dir/scripts"
  local script_file
  local -i script_index function_index
  local output_fd

  mkdir -p -- "$scripts_dir" || return
  for (( script_index = 1; script_index <= script_count; script_index++ )); do
    script_file=$(printf '%s/fixture-%03d.zsh' "$scripts_dir" "$script_index")
    exec {output_fd}>| "$script_file" || return
    print -u "$output_fd" -r -- "# Deterministic parse-heavy benchmark fixture."
    for (( function_index = 1; function_index <= function_count; function_index++ )); do
      writef "$output_fd" 'zpmod_benchmark_%03d_%03d() {\n' "$script_index" "$function_index"
      print -u "$output_fd" -r -- "  builtin emulate -L zsh"
      writef "$output_fd" '  local -i value=%d\n' "$function_index"
      writef "$output_fd" '  (( value += %d ))\n' "$script_index"
      print -u "$output_fd" -r -- '  [[ $value -gt 0 ]]'
      print -u "$output_fd" -r -- "}"
    done
    exec {output_fd}>&-
  done
}

typeset plain_dir="$benchmark_root/plain"
typeset manual_dir="$benchmark_root/manual-zwc"
typeset cold_dir="$benchmark_root/zpmod-cold"
typeset warm_dir="$benchmark_root/zpmod-warm"
generate_workload "$plain_dir" || fail "could not generate plain workload"
generate_workload "$manual_dir" || fail "could not generate manual .zwc workload"
generate_workload "$cold_dir" || fail "could not generate zpmod cold workload"
generate_workload "$warm_dir" || fail "could not generate zpmod warm workload"

run_workload_shell() {
  builtin emulate -L zsh
  local case_name=$1
  local scripts_dir
  local -a source_files=()

  case $case_name in
    plain) scripts_dir="$plain_dir/scripts" ;;
    manual_zwc) scripts_dir="$manual_dir/scripts" ;;
    zpmod_cold) scripts_dir="$cold_dir/scripts" ;;
    zpmod_warm) scripts_dir="$warm_dir/scripts" ;;
    *) return 64 ;;
  esac
  source_files=( "$scripts_dir"/*.zsh(N) )
  (( ${#source_files} == script_count )) || return 65

  if [[ $case_name == zpmod_* ]]; then
    ZDOTDIR="$isolated_zdotdir" HOME="$isolated_home" zsh -f -c '
      module_path=( "$1" "${module_path[@]}" )
      zmodload -i zpmod || exit 70
      shift
      for script_file in "$@"; do
        source "$script_file" || exit 71
      done
    ' zsh "$module_dir" "${source_files[@]}"
  else
    ZDOTDIR="$isolated_zdotdir" HOME="$isolated_home" zsh -f -c '
      for script_file in "$@"; do
        source "$script_file" || exit 71
      done
    ' zsh "${source_files[@]}"
  fi
}

typeset script_file
for script_file in "$manual_dir"/scripts/*.zsh(N); do
  ZDOTDIR="$isolated_zdotdir" HOME="$isolated_home" zsh -f -c 'zcompile -U "$1.zwc" "$1"' zsh "$script_file" >/dev/null 2>&1 ||
    fail "manual zcompile failed for ${script_file:t}"
done

run_workload_shell zpmod_warm >/dev/null 2>&1 ||
  fail "zpmod warm-up compilation failed"

typeset -a manual_zwc_files=( "$manual_dir"/scripts/*.zwc(N) )
typeset -a warm_zwc_files=( "$warm_dir"/scripts/*.zwc(N) )
(( ${#manual_zwc_files} == script_count )) || fail "manual zcompile produced ${#manual_zwc_files} of $script_count expected files"
(( ${#warm_zwc_files} == script_count )) || fail "zpmod produced ${#warm_zwc_files} of $script_count expected files"

prepare_case() {
  builtin emulate -L zsh
  local case_name=$1
  local -a stale_files=()

  if [[ $case_name == zpmod_cold ]]; then
    stale_files=( "$cold_dir"/scripts/*.zwc(N) )
    (( ${#stale_files} == 0 )) || rm -f -- "${stale_files[@]}" || return
  fi
}

typeset -F 6 measured_ms
measure_case() {
  builtin emulate -L zsh
  local case_name=$1
  local -a compiled_files=()
  local -F 9 started_at finished_at

  prepare_case "$case_name" || return
  started_at=$EPOCHREALTIME
  run_workload_shell "$case_name" >/dev/null 2>&1 || return
  finished_at=$EPOCHREALTIME
  if [[ $case_name == zpmod_cold ]]; then
    compiled_files=( "$cold_dir"/scripts/*.zwc(N) )
    (( ${#compiled_files} == script_count )) || return 66
  fi
  (( measured_ms = (finished_at - started_at) * 1000.0 ))
}

typeset -a case_names=( plain zpmod_cold zpmod_warm manual_zwc )
typeset -A case_labels=(
  plain "Plain source"
  zpmod_cold "zpmod first run"
  zpmod_warm "zpmod warm"
  manual_zwc "Manual .zwc"
)
typeset -a samples_plain=()
typeset -a samples_zpmod_cold=()
typeset -a samples_zpmod_warm=()
typeset -a samples_manual_zwc=()
typeset -i round slot case_index
typeset case_name

for (( round = 1; round <= warmup_count; round++ )); do
  for (( slot = 1; slot <= ${#case_names}; slot++ )); do
    case_index=$(( ((round + slot - 2) % ${#case_names}) + 1 ))
    case_name=${case_names[$case_index]}
    measure_case "$case_name" || fail "warm-up failed for $case_name"
  done
done

for (( round = 1; round <= run_count; round++ )); do
  for (( slot = 1; slot <= ${#case_names}; slot++ )); do
    case_index=$(( ((round + slot - 2) % ${#case_names}) + 1 ))
    case_name=${case_names[$case_index]}
    measure_case "$case_name" || fail "measurement failed for $case_name"
    case $case_name in
      plain) samples_plain+=( "$measured_ms" ) ;;
      zpmod_cold) samples_zpmod_cold+=( "$measured_ms" ) ;;
      zpmod_warm) samples_zpmod_warm+=( "$measured_ms" ) ;;
      manual_zwc) samples_manual_zwc+=( "$measured_ms" ) ;;
    esac
  done
done

typeset -A result_median=()
typeset -A result_p95=()
typeset -A result_mean=()
typeset -A result_stdev=()
typeset -A result_min=()
typeset -A result_max=()
typeset -A result_samples=()

summarize_case() {
  builtin emulate -L zsh
  local case_key=$1
  shift
  local -a values=( "$@" )
  local -a sorted_values=( ${(on)values} )
  local -i count=${#sorted_values}
  local -i midpoint p95_index
  local value
  local -F 9 sum=0 mean=0 median=0 p95=0 variance=0 difference=0

  (( count > 0 )) || return 1
  for value in "${values[@]}"; do
    (( sum += value ))
  done
  (( mean = sum / count ))
  if (( count % 2 )); then
    midpoint=$(( (count + 1) / 2 ))
    median=${sorted_values[$midpoint]}
  else
    midpoint=$(( count / 2 ))
    (( median = (sorted_values[$midpoint] + sorted_values[$(( midpoint + 1 ))]) / 2.0 ))
  fi
  p95_index=$(( (count * 95 + 99) / 100 ))
  p95=${sorted_values[$p95_index]}
  for value in "${values[@]}"; do
    (( difference = value - mean ))
    (( variance += difference * difference ))
  done
  (( variance = variance / count ))

  result_median[$case_key]=$(printf '%.3f' "$median")
  result_p95[$case_key]=$(printf '%.3f' "$p95")
  result_mean[$case_key]=$(printf '%.3f' "$mean")
  result_stdev[$case_key]=$(printf '%.3f' "$(( sqrt(variance) ))")
  result_min[$case_key]=$(printf '%.3f' "${sorted_values[1]}")
  result_max[$case_key]=$(printf '%.3f' "${sorted_values[-1]}")
  result_samples[$case_key]=${(j:,:)values}
}

summarize_case plain "${samples_plain[@]}" || fail "could not summarize plain case"
summarize_case zpmod_cold "${samples_zpmod_cold[@]}" || fail "could not summarize zpmod cold case"
summarize_case zpmod_warm "${samples_zpmod_warm[@]}" || fail "could not summarize zpmod warm case"
summarize_case manual_zwc "${samples_manual_zwc[@]}" || fail "could not summarize manual .zwc case"

typeset -A result_relative=()
typeset -F 9 plain_median=${result_median[plain]}
for case_name in "${case_names[@]}"; do
  result_relative[$case_name]=$(printf '%.3f' "$(( result_median[$case_name] / plain_median ))")
done

typeset captured_at
typeset os_name
typeset architecture
typeset kernel_release
typeset zsh_version
typeset zpmod_version
typeset compiler_output
typeset compiler_label="unavailable"
typeset module_digest
captured_at=$(date -u '+%Y-%m-%dT%H:%M:%SZ') || fail "could not capture UTC timestamp"
os_name=$(uname -s) || fail "could not detect operating system"
architecture=$(uname -m) || fail "could not detect architecture"
kernel_release=$(uname -r) || fail "could not detect kernel release"
zsh_version=$(zsh --version) || fail "could not detect Zsh version"
zpmod_version=$(ZDOTDIR="$isolated_zdotdir" HOME="$isolated_home" zsh -f -c 'module_path=( "$1" "${module_path[@]}" ); zmodload -i zpmod || exit; zpmod -V' zsh "$module_dir") ||
  fail "could not detect zpmod version"
if [[ -n $compiler_override ]]; then
  compiler_label=$compiler_override
elif (( $+commands[cc] )); then
  compiler_output=$(cc --version 2>/dev/null) || true
  [[ -n $compiler_output ]] && compiler_label=${compiler_output%%$'\n'*}
fi
if [[ -z $cpu_label ]] && (( $+commands[lscpu] )); then
  typeset cpu_output
  typeset cpu_line
  cpu_output=$(lscpu 2>/dev/null) || true
  cpu_line=${${(M)${(f)cpu_output}:#Model name:*}[1]}
  cpu_label=${${cpu_line#*:}##[[:space:]]#}
fi
if [[ -z $cpu_label ]] && (( $+commands[sysctl] )); then
  cpu_label=$(sysctl -n machdep.cpu.brand_string 2>/dev/null) || true
fi
[[ -n $cpu_label ]] || cpu_label="unavailable"
require_metadata_scalar detected-cpu "$cpu_label"
require_metadata_scalar detected-compiler "$compiler_label"
module_digest=$(module_sha256 "$module_file") || fail "could not hash zpmod module"
[[ -n $environment_label ]] || environment_label="$os_name $architecture"

typeset tsv_path="$output_dir/benchmark.tsv"
typeset json_path="$output_dir/benchmark.json"
typeset output_fd
exec {output_fd}>| "$tsv_path" || fail "could not write $tsv_path"
writef "$output_fd" 'meta\tschema_version\t1\n'
writef "$output_fd" 'meta\tcaptured_at\t%s\n' "$captured_at"
writef "$output_fd" 'meta\tsource_revision\t%s\n' "$source_revision"
writef "$output_fd" 'meta\tenvironment\t%s\n' "$environment_label"
writef "$output_fd" 'meta\tos\t%s\n' "$os_name"
writef "$output_fd" 'meta\tarchitecture\t%s\n' "$architecture"
writef "$output_fd" 'meta\tkernel_release\t%s\n' "$kernel_release"
writef "$output_fd" 'meta\tcpu\t%s\n' "$cpu_label"
writef "$output_fd" 'meta\tzsh_version\t%s\n' "$zsh_version"
writef "$output_fd" 'meta\tzpmod_version\t%s\n' "$zpmod_version"
writef "$output_fd" 'meta\tcompiler\t%s\n' "$compiler_label"
writef "$output_fd" 'meta\tmodule_sha256\t%s\n' "$module_digest"
writef "$output_fd" 'meta\tmodule_origin\t%s\n' "$module_origin"
writef "$output_fd" 'meta\torigin_sha256\t%s\n' "$origin_sha256"
writef "$output_fd" 'meta\tscripts\t%d\n' "$script_count"
writef "$output_fd" 'meta\tfunctions_per_script\t%d\n' "$function_count"
writef "$output_fd" 'meta\twarmups\t%d\n' "$warmup_count"
writef "$output_fd" 'meta\truns\t%d\n' "$run_count"
for case_name in "${case_names[@]}"; do
  writef "$output_fd" 'result\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$case_name" "${case_labels[$case_name]}" "${result_median[$case_name]}" "${result_p95[$case_name]}" \
    "${result_mean[$case_name]}" "${result_stdev[$case_name]}" "${result_min[$case_name]}" \
    "${result_max[$case_name]}" "${result_relative[$case_name]}" "${result_samples[$case_name]}"
done
exec {output_fd}>&-

exec {output_fd}>| "$json_path" || fail "could not write $json_path"
print -u "$output_fd" -r -- '{'
print -u "$output_fd" -r -- '  "schema_version": 1,'
writef "$output_fd" '  "captured_at": "'; json_escape "$captured_at" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '  "source_revision": "'; json_escape "$source_revision" >&$output_fd; print -u "$output_fd" -r -- '",'
print -u "$output_fd" -r -- '  "environment": {'
writef "$output_fd" '    "label": "'; json_escape "$environment_label" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '    "os": "'; json_escape "$os_name" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '    "architecture": "'; json_escape "$architecture" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '    "kernel_release": "'; json_escape "$kernel_release" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '    "cpu": "'; json_escape "$cpu_label" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '    "zsh_version": "'; json_escape "$zsh_version" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '    "zpmod_version": "'; json_escape "$zpmod_version" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '    "compiler": "'; json_escape "$compiler_label" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '    "module_sha256": "'; json_escape "$module_digest" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '    "module_origin": "'; json_escape "$module_origin" >&$output_fd; print -u "$output_fd" -r -- '",'
writef "$output_fd" '    "origin_sha256": "'; json_escape "$origin_sha256" >&$output_fd; print -u "$output_fd" -r -- '"'
print -u "$output_fd" -r -- '  },'
print -u "$output_fd" -r -- '  "workload": {'
writef "$output_fd" '    "scripts": %d,\n' "$script_count"
writef "$output_fd" '    "functions_per_script": %d,\n' "$function_count"
writef "$output_fd" '    "warmups": %d,\n' "$warmup_count"
writef "$output_fd" '    "runs": %d,\n' "$run_count"
print -u "$output_fd" -r -- '    "order": "balanced rotation"'
print -u "$output_fd" -r -- '  },'
print -u "$output_fd" -r -- '  "cases": ['
typeset -i case_position=0
typeset -a sample_values=()
typeset sample_value
for case_name in "${case_names[@]}"; do
  (( case_position++ )) || true
  print -u "$output_fd" -r -- '    {'
  writef "$output_fd" '      "id": "'; json_escape "$case_name" >&$output_fd; print -u "$output_fd" -r -- '",'
  writef "$output_fd" '      "label": "'; json_escape "${case_labels[$case_name]}" >&$output_fd; print -u "$output_fd" -r -- '",'
  writef "$output_fd" '      "median_ms": %s,\n' "${result_median[$case_name]}"
  writef "$output_fd" '      "p95_ms": %s,\n' "${result_p95[$case_name]}"
  writef "$output_fd" '      "mean_ms": %s,\n' "${result_mean[$case_name]}"
  writef "$output_fd" '      "stdev_ms": %s,\n' "${result_stdev[$case_name]}"
  writef "$output_fd" '      "min_ms": %s,\n' "${result_min[$case_name]}"
  writef "$output_fd" '      "max_ms": %s,\n' "${result_max[$case_name]}"
  writef "$output_fd" '      "relative_to_plain": %s,\n' "${result_relative[$case_name]}"
  writef "$output_fd" '      "samples_ms": ['
  sample_values=( ${(s:,:)result_samples[$case_name]} )
  typeset -i sample_position=0
  for sample_value in "${sample_values[@]}"; do
    (( sample_position++ )) || true
    (( sample_position > 1 )) && writef "$output_fd" ', '
    writef "$output_fd" '%s' "$sample_value"
  done
  print -u "$output_fd" -r -- ']'
  if (( case_position < ${#case_names} )); then
    print -u "$output_fd" -r -- '    },'
  else
    print -u "$output_fd" -r -- '    }'
  fi
done
print -u "$output_fd" -r -- '  ]'
print -u "$output_fd" -r -- '}'
exec {output_fd}>&-

print -r -- "Wrote $json_path"
print -r -- "Wrote $tsv_path"
