#!/usr/bin/env zsh
# SPDX-License-Identifier: MIT

emulate -R zsh
setopt pipe_fail extended_glob

usage() {
  print -r -- "Usage: benchmarks/render.zsh --input FILE --svg FILE --markdown FILE"
}

fail() {
  print -ru2 -- "benchmark renderer: $*"
  exit 1
}

writef() {
  local output_descriptor=$1
  shift
  printf "$@" >&$output_descriptor
}

xml_escape() {
  local value=$1
  value=${value//&/&amp;}
  value=${value//</&lt;}
  value=${value//>/&gt;}
  value=${value//\"/&quot;}
  value=${value//\'/&apos;}
  print -rn -- "$value"
}

is_nonnegative_decimal() {
  local value=$1
  [[ $value == <-> || $value == <->.<-> ]]
}

typeset input_path=""
typeset svg_path=""
typeset markdown_path=""

while (( $# )); do
  case $1 in
    --input)
      (( $# >= 2 )) || fail "--input requires a value"
      input_path=$2
      shift 2
      ;;
    --svg)
      (( $# >= 2 )) || fail "--svg requires a value"
      svg_path=$2
      shift 2
      ;;
    --markdown)
      (( $# >= 2 )) || fail "--markdown requires a value"
      markdown_path=$2
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

[[ -f $input_path ]] || fail "input file does not exist: $input_path"
[[ -n $svg_path ]] || fail "--svg is required"
[[ -n $markdown_path ]] || fail "--markdown is required"
mkdir -p -- "${svg_path:h}" "${markdown_path:h}" || fail "could not create output directories"

typeset -A metadata=()
typeset -a case_ids=()
typeset -A labels=()
typeset -A medians=()
typeset -A p95_values=()
typeset -A means=()
typeset -A stdevs=()
typeset -A minimums=()
typeset -A maximums=()
typeset -A relatives=()
typeset -A expected_labels=(
  plain "Plain source"
  zpmod_cold "zpmod first run"
  zpmod_warm "zpmod warm"
  manual_zwc "Manual .zwc"
)
typeset -A seen_cases=()
typeset record_type key field3 field4 field5 field6 field7 field8 field9 field10 field11

while IFS=$'\t' read -r record_type key field3 field4 field5 field6 field7 field8 field9 field10 field11; do
  case $record_type in
    meta)
      [[ $key == [a-z0-9_]## ]] || fail "invalid metadata key: $key"
      [[ -z $field4 && -z $field5 && -z $field6 && -z $field7 && -z $field8 && -z $field9 && -z $field10 && -z $field11 ]] ||
        fail "metadata record contains unexpected fields: $key"
      [[ $field3 != *$'\n'* && $field3 != *$'\r'* && $field3 != *'`'* ]] || fail "unsafe metadata value for $key"
      metadata[$key]=$field3
      ;;
    result)
      [[ -n ${expected_labels[$key]:-} ]] || fail "unknown benchmark case: $key"
      [[ -z ${seen_cases[$key]:-} ]] || fail "duplicate benchmark case: $key"
      [[ $field3 == "${expected_labels[$key]}" ]] || fail "unexpected label for $key"
      is_nonnegative_decimal "$field4" || fail "invalid median for $key"
      is_nonnegative_decimal "$field5" || fail "invalid p95 for $key"
      is_nonnegative_decimal "$field6" || fail "invalid mean for $key"
      is_nonnegative_decimal "$field7" || fail "invalid standard deviation for $key"
      is_nonnegative_decimal "$field8" || fail "invalid minimum for $key"
      is_nonnegative_decimal "$field9" || fail "invalid maximum for $key"
      is_nonnegative_decimal "$field10" || fail "invalid relative value for $key"
      seen_cases[$key]=1
      case_ids+=( "$key" )
      labels[$key]=$field3
      medians[$key]=$field4
      p95_values[$key]=$field5
      means[$key]=$field6
      stdevs[$key]=$field7
      minimums[$key]=$field8
      maximums[$key]=$field9
      relatives[$key]=$field10
      ;;
    "") ;;
    *) fail "unknown record type in $input_path: $record_type" ;;
  esac
done < "$input_path"

(( ${#case_ids} > 0 )) || fail "input contains no benchmark results"
(( ${#case_ids} == ${#expected_labels} )) || fail "input must contain all four benchmark cases"

typeset -F 6 chart_max=0
typeset case_id
for case_id in "${case_ids[@]}"; do
  (( p95_values[$case_id] > chart_max )) && chart_max=${p95_values[$case_id]}
done
(( chart_max > 0 )) || fail "all p95 measurements are zero"

typeset -i svg_width=960
typeset -i svg_height=$(( 150 + ${#case_ids} * 72 ))
typeset -i plot_x=245
typeset -i plot_width=590
typeset -i row_y=118
typeset -i row_gap=72
typeset -i bar_height=24
typeset -F 6 bar_width marker_x
typeset output_fd

exec {output_fd}>| "$svg_path" || fail "could not write $svg_path"
print -u "$output_fd" -r -- '<?xml version="1.0" encoding="UTF-8"?>'
writef "$output_fd" '<svg xmlns="http://www.w3.org/2000/svg" role="img" aria-labelledby="title description" viewBox="0 0 %d %d">\n' "$svg_width" "$svg_height"
print -u "$output_fd" -r -- '  <title id="title">zpmod startup benchmark</title>'
writef "$output_fd" '  <desc id="description">Median startup time bars and p95 markers for plain source, zpmod first run, zpmod warm, and manual compiled Zsh workloads. Exact values follow the chart in the benchmark report.</desc>\n'
print -u "$output_fd" -r -- '  <rect width="100%" height="100%" fill="#ffffff"/>'
print -u "$output_fd" -r -- '  <style>text{font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;fill:#172033}.title{font-family:system-ui,sans-serif;font-weight:700}.subtitle{font-family:system-ui,sans-serif;fill:#4b5563}.track{fill:#eef2f7}.bar{fill:#1769aa}.marker{stroke:#d1495b;stroke-width:4}.axis{stroke:#8091a5;stroke-width:1}.value{font-weight:700}</style>'
print -u "$output_fd" -r -- '  <text class="title" x="30" y="38" font-size="24">Startup cost by compilation mode</text>'
writef "$output_fd" '  <text class="subtitle" x="30" y="66" font-size="14">%s, %s runs, lower is better</text>\n' "$(xml_escape "${metadata[environment]:-unspecified environment}")" "$(xml_escape "${metadata[runs]:-unknown}")"
print -u "$output_fd" -r -- '  <rect x="30" y="82" width="16" height="12" fill="#1769aa"/><text x="54" y="93" font-size="13">median</text>'
print -u "$output_fd" -r -- '  <line class="marker" x1="132" x2="132" y1="80" y2="96"/><text x="142" y="93" font-size="13">p95</text>'
writef "$output_fd" '  <line class="axis" x1="%d" x2="%d" y1="104" y2="104"/>\n' "$plot_x" "$(( plot_x + plot_width ))"

for case_id in "${case_ids[@]}"; do
  (( bar_width = medians[$case_id] / chart_max * plot_width ))
  (( marker_x = plot_x + p95_values[$case_id] / chart_max * plot_width ))
  writef "$output_fd" '  <text x="30" y="%d" font-size="15">%s</text>\n' "$(( row_y + 18 ))" "$(xml_escape "${labels[$case_id]}")"
  writef "$output_fd" '  <rect class="track" x="%d" y="%d" width="%d" height="%d" rx="4"/>\n' "$plot_x" "$row_y" "$plot_width" "$bar_height"
  writef "$output_fd" '  <rect class="bar" x="%d" y="%d" width="%.2f" height="%d" rx="4"/>\n' "$plot_x" "$row_y" "$bar_width" "$bar_height"
  writef "$output_fd" '  <line class="marker" x1="%.2f" x2="%.2f" y1="%d" y2="%d"/>\n' "$marker_x" "$marker_x" "$(( row_y - 4 ))" "$(( row_y + bar_height + 4 ))"
  writef "$output_fd" '  <text class="value" x="850" y="%d" font-size="14">%s ms</text>\n' "$(( row_y + 18 ))" "${medians[$case_id]}"
  (( row_y += row_gap ))
done

writef "$output_fd" '  <text class="subtitle" x="30" y="%d" font-size="12">Bars show medians. Red markers show p95. See benchmark.md for raw context and exact values.</text>\n' "$(( svg_height - 20 ))"
print -u "$output_fd" -r -- '</svg>'
exec {output_fd}>&-

exec {output_fd}>| "$markdown_path" || fail "could not write $markdown_path"
print -u "$output_fd" -r -- '# zpmod startup benchmark'
print -u "$output_fd" -r -- ''
print -u "$output_fd" -r -- 'This report compares an identical generated workload across four compilation modes. Lower times are better.'
print -u "$output_fd" -r -- ''
print -u "$output_fd" -r -- '| Mode | Median | p95 | Standard deviation | Relative to plain |'
print -u "$output_fd" -r -- '| --- | ---: | ---: | ---: | ---: |'
for case_id in "${case_ids[@]}"; do
  writef "$output_fd" '| %s | %s ms | %s ms | %s ms | %sx |\n' \
    "${labels[$case_id]}" "${medians[$case_id]}" "${p95_values[$case_id]}" "${stdevs[$case_id]}" "${relatives[$case_id]}"
done
print -u "$output_fd" -r -- ''
print -u "$output_fd" -r -- '## Environment'
print -u "$output_fd" -r -- ''
writef "$output_fd" -- '- Captured: `%s`\n' "${metadata[captured_at]:-unknown}"
writef "$output_fd" -- '- Source revision: `%s`\n' "${metadata[source_revision]:-unknown}"
writef "$output_fd" -- '- Environment: `%s`\n' "${metadata[environment]:-unknown}"
writef "$output_fd" -- '- Operating system and architecture: `%s %s`\n' "${metadata[os]:-unknown}" "${metadata[architecture]:-unknown}"
writef "$output_fd" -- '- Kernel: `%s`\n' "${metadata[kernel_release]:-unknown}"
writef "$output_fd" -- '- CPU: `%s`\n' "${metadata[cpu]:-unknown}"
writef "$output_fd" -- '- Zsh: `%s`\n' "${metadata[zsh_version]:-unknown}"
writef "$output_fd" -- '- zpmod: `%s`\n' "${metadata[zpmod_version]:-unknown}"
writef "$output_fd" -- '- Compiler: `%s`\n' "${metadata[compiler]:-unknown}"
writef "$output_fd" -- '- Module SHA-256: `%s`\n' "${metadata[module_sha256]:-unknown}"
writef "$output_fd" -- '- Module origin: `%s`\n' "${metadata[module_origin]:-unknown}"
writef "$output_fd" -- '- Origin SHA-256: `%s`\n' "${metadata[origin_sha256]:-unknown}"
writef "$output_fd" -- '- Workload: `%s` scripts with `%s` generated functions each\n' "${metadata[scripts]:-unknown}" "${metadata[functions_per_script]:-unknown}"
writef "$output_fd" -- '- Sampling: `%s` warmups and `%s` measured runs per case\n' "${metadata[warmups]:-unknown}" "${metadata[runs]:-unknown}"
print -u "$output_fd" -r -- ''
print -u "$output_fd" -r -- '## Interpretation'
print -u "$output_fd" -r -- ''
print -u "$output_fd" -r -- '- **Plain source** parses uncompiled files without zpmod.'
print -u "$output_fd" -r -- '- **zpmod first run** starts without `.zwc` files and includes automatic compilation cost.'
print -u "$output_fd" -r -- '- **zpmod warm** reuses `.zwc` files generated by an earlier zpmod run.'
print -u "$output_fd" -r -- '- **Manual `.zwc`** is the native Zsh control, compiled before measurement and loaded without zpmod.'
print -u "$output_fd" -r -- ''
print -u "$output_fd" -r -- 'These results describe this synthetic workload and environment. They are not a universal startup-speed claim.'
exec {output_fd}>&-

print -r -- "Wrote $svg_path"
print -r -- "Wrote $markdown_path"
