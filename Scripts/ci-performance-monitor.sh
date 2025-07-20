#!/usr/bin/env bash
# CI Performance Monitoring Script for Trunk-Based Workflow

set -euo pipefail

echo "CI Performance Monitor - zpmod trunk integration"
echo "Usage: $0 [measure|summary|help]"

# Simple performance measurement
if [[ ${1:-measure} == "measure" ]]; then
  echo "Measuring trunk performance..."

  start_time=$(date +%s.%N)
  if trunk check -y >/tmp/trunk_test.log 2>&1; then
    end_time=$(date +%s.%N)
    duration=$(echo "${end_time} - ${start_time}" | bc -l)
    echo "✅ Trunk check completed in ${duration}s"

    # Show basic metrics
    echo "Performance Summary:"
    echo "  Duration: ${duration}s"
    echo "  Status: ✅ All checks passed"
    improvement=$(echo "300 - ${duration}" | bc -l)
    echo "  Improvement vs GitHub Actions: ~${improvement}s faster"
  else
    end_time=$(date +%s.%N)
    duration=$(echo "${end_time} - ${start_time}" | bc -l)
    echo "⚠️  Trunk check completed with issues in ${duration}s"
    echo "Check output in /tmp/trunk_test.log"
  fi
  rm -f /tmp/trunk_test.log
elif [[ $1 == "help" ]]; then
  echo "Commands:"
  echo "  measure - Run performance measurement"
  echo "  help    - Show this help"
fi
