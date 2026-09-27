#!/usr/bin/env python3
"""ADR-0024 comparison around zpmod's existing four-mode workload, Python 3.8+."""
import argparse
import json
import math
import os
from pathlib import Path
import re
import statistics
import subprocess
import sys

VARIANTS = ("baseline", "candidate", "control")
ORDER = (("baseline", "candidate", "control"), ("candidate", "control", "baseline"),
         ("control", "baseline", "candidate"), ("baseline", "control", "candidate"),
         ("control", "candidate", "baseline"), ("candidate", "baseline", "control"))


def summary(samples):
    if not samples or any(type(v) not in (float, int) or not math.isfinite(v) or v < 0 for v in samples):
        raise ValueError("invalid raw duration")
    return {"median": statistics.median(samples), "p95": sorted(samples)[math.ceil(.95 * len(samples)) - 1],
            "min": min(samples), "count": len(samples), "samples": samples}


def row(before, after):
    change = {}
    for metric in ("median", "p95"):
        delta = after[metric] - before[metric]
        change[metric + "_delta_ms"] = delta
        change[metric + "_delta_percent"] = 100 * delta / before[metric] if before[metric] else None
    flag = any(change[key] is not None and change[key] > threshold for key, threshold in
               [("median_delta_percent", 10), ("p95_delta_percent", 15)])
    return {"results": {"baseline": before, "candidate": after}, "change": change, "flag": flag}


def report_identity(raw, label, image, runs):
    env = raw["environment"]
    return {"label": label, "source_revision": raw["source_revision"],
            "environment": {"zsh_version": env["zsh_version"], "architecture": env["architecture"],
                            "cpu": env["cpu"], "runner_image": image},
            "workload": {"samples": runs, "warmups": raw["workload"]["warmups"],
                         "scripts": raw["workload"]["scripts"],
                         "functions_per_script": raw["workload"]["functions_per_script"]},
            "module_sha256": env["module_sha256"]}


def comparable(before, after):
    return before["environment"] == after["environment"] and before["workload"] == after["workload"]


def comparison(reports, image, runs):
    identities = {label: report_identity(records[0], label, image, runs) for label, records in reports.items()}
    case_ids = {case["id"] for case in reports["baseline"][0]["cases"]}
    gathered = {label: {case: [] for case in case_ids} for label in VARIANTS}
    for label, records in reports.items():
        if len(records) != runs:
            raise ValueError("incomplete variant sample count")
        for raw in records:
            if report_identity(raw, label, image, runs) != identities[label]:
                raise ValueError("variant identity changed during measurement")
            if {case["id"] for case in raw["cases"]} != case_ids:
                raise ValueError("case inventory changed")
            for case in raw["cases"]:
                if len(case["samples_ms"]) != 1:
                    raise ValueError("each balanced trial must contribute exactly one sample")
                gathered[label][case["id"]].extend(case["samples_ms"])
    base = identities["baseline"]
    if not all(comparable(base, identities[label]) for label in VARIANTS):
        raise ValueError("incompatible same-run environment or sample counts")
    if identities["control"]["source_revision"] != base["source_revision"] or identities["control"]["module_sha256"] != base["module_sha256"]:
        raise ValueError("A/A control does not identify the baseline module")
    cases = {}
    controls = {}
    for case in sorted(case_ids):
        before = summary(gathered["baseline"][case])
        cases[case] = row(before, summary(gathered["candidate"][case]))
        controls[case] = row(before, summary(gathered["control"][case]))
    return {"schema_version": 1, "baseline": base, "candidate": identities["candidate"],
            "control_identity": identities["control"], "comparable": True,
            "cases": cases, "control": controls, "thresholds": {"median_percent": 10, "p95_percent": 15},
            "flagged": [case for case, value in cases.items() if value["flag"]], "failed": [],
            "methodology": {"variant_order": "six rotating permutations", "mode_order": "canonical balanced rotation",
                            "warmup_scope": "per fresh harness invocation", "workload": "synthetic four-mode startup"}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module-dir", required=True, type=Path)
    parser.add_argument("--baseline-module-dir", type=Path)
    parser.add_argument("--source-revision", default=os.environ.get("ZD_SOURCE_REVISION"), required="ZD_SOURCE_REVISION" not in os.environ)
    parser.add_argument("--baseline-source-revision")
    parser.add_argument("--runner-image", required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--warmups", type=int, default=5)
    parser.add_argument("--runs", type=int, default=30)
    parser.add_argument("--scripts", type=int, default=40)
    parser.add_argument("--functions", type=int, default=30)
    args = parser.parse_args()
    if args.runs < 3:
        parser.error("at least three rounds are required to rotate every variant position")
    if min(args.warmups, args.scripts, args.functions) < 1:
        parser.error("all sample and workload counts must be positive")
    if args.baseline_module_dir and not args.baseline_source_revision:
        parser.error("a separate baseline module requires its source revision")
    args.baseline_source_revision = args.baseline_source_revision or args.source_revision
    if not all(re.fullmatch(r"[0-9a-f]{40}", revision) for revision in [args.source_revision, args.baseline_source_revision]):
        parser.error("source identities must be full commit SHAs")
    if args.output_dir.exists() and any(args.output_dir.iterdir()):
        parser.error("output directory must be empty")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    reports = {label: [] for label in VARIANTS}
    harness = Path(__file__).with_name("run.zsh")
    try:
        for iteration in range(args.runs):
            for label in ORDER[iteration % len(ORDER)]:
                module = args.module_dir if label == "candidate" else (args.baseline_module_dir or args.module_dir)
                revision = args.source_revision if label == "candidate" else args.baseline_source_revision
                output = args.output_dir / "raw" / f"{iteration + 1:03d}-{label}"
                command = ["zsh", "-f", str(harness), "--module-dir", str(module), "--output-dir", str(output),
                           "--source-revision", revision, "--environment", args.runner_image, "--warmups", str(args.warmups),
                           "--runs", "1", "--scripts", str(args.scripts), "--functions", str(args.functions)]
                output.mkdir(parents=True)
                with (output / "harness.log").open("w") as log:
                    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
                reports[label].append(json.loads((output / "benchmark.json").read_text()))
        result = comparison(reports, args.runner_image, args.runs)
        (args.output_dir / "comparison.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
        print("Comparison completed; timing flags:", ", ".join(result["flagged"]) or "none")
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        (args.output_dir / "failure.json").write_text(json.dumps({"status": "invalid", "reason": str(error)}) + "\n")
        print("No accepted timing comparison:", error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
