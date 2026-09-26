#!/usr/bin/env python3
"""Validate real source-study JSON with the standard-library JSON parser."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

parser = argparse.ArgumentParser()
parser.add_argument("--zsh", required=True)
parser.add_argument("--module-dir", required=True)
parser.add_argument("--build-dir", required=True)
parser.add_argument("--clock", choices=("auto", "monotonic", "wall"), required=True)
args, remaining = parser.parse_known_args()


class SourceStudyJSON(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="zpmod-json-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.child = self.root / 'quote"slash\\line\n.zsh'
        self.child.write_text(":\n")
        (self.root / "nested.zsh").write_text('source "$2"\n')
        (self.root / "failure.zsh").write_text("return 7\n")
        (self.root / "parse-error.zsh").write_text("if then\n")
        (self.root / "change-dir.zsh").write_text("cd /\n")

    def shell(self, script, module_dir=None):
        command = [args.zsh, "-f", "-c",
                   'emulate -R zsh; module_path=("$1" $module_path); zmodload zpmod || exit 90; ' + script,
                   "--", str(module_dir or args.module_dir), str(self.root), str(self.child)]
        result = subprocess.run(command, capture_output=True, check=False,
                                env={**os.environ, "HOME": str(self.root), "ZDOTDIR": str(self.root), "LC_ALL": "C"})
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        report = json.loads(result.stdout)
        self.assertEqual(report["schema_version"], 1)
        self.assertIs(report["inclusive"], True)
        self.assertEqual(report["order"], "completion")
        self.assertEqual(report["execution_mode"], "instrumented-auto-compile")
        self.assertEqual(report["unit"], "nanoseconds")
        self.assertIs(type(report["monotonic"]), bool)
        if args.clock != "auto":
            self.assertEqual(report["monotonic"], args.clock == "monotonic")
        self.assertIn(report["clock"], ("CLOCK_MONOTONIC", "CLOCK_MONOTONIC_RAW") if report["monotonic"] else ("gettimeofday",))
        return report

    @staticmethod
    def paths(report):
        return [bytes.fromhex(event["path_hex"]) for event in report["events"]]

    def test_empty_and_nested_completion_order(self):
        empty = self.shell("zpmod source-study --json")
        self.assertEqual(empty["status"], "empty")
        self.assertEqual(empty["events"], [])
        report = self.shell('source "$2/nested.zsh" "$2" "$3"; zpmod source-study --json')
        self.assertEqual(report["status"], "complete")
        self.assertEqual(self.paths(report), [os.fsencode(self.child), os.fsencode(self.root / "nested.zsh")])
        self.assertEqual([e["id"] for e in report["events"]], [1, 2])
        self.assertEqual([e["depth"] for e in report["events"]], [2, 1])
        for event in report["events"]:
            self.assertEqual(event["source_status"], 0)
            self.assertEqual(event["exit_status"], 0)
            self.assertIs(type(event["duration_ns"]), int)
            self.assertGreaterEqual(event["duration_ns"], 0)
        self.assertGreaterEqual(report["events"][1]["duration_ns"], report["events"][0]["duration_ns"])

    def test_failed_and_repeated_sources(self):
        report = self.shell('source "$2/failure.zsh"; [[ $? == 7 ]] || exit 91; source "$3"; source "$3"; source "$2/missing.zsh" 2>/dev/null; [[ $? != 0 ]] || exit 92; zpmod source-study --json')
        self.assertEqual(len(report["events"]), 4)
        self.assertEqual(report["events"][0]["exit_status"], 7)
        self.assertEqual(report["events"][0]["source_status"], 0)
        self.assertEqual(report["events"][-1]["source_status"], 1)
        self.assertEqual(report["events"][-1]["exit_status"], 127)
        self.assertEqual(self.paths(report)[1:3], [os.fsencode(self.child)] * 2)
        self.assertEqual([e["id"] for e in report["events"]], [1, 2, 3, 4])

    def test_parse_failure_and_history_preservation(self):
        report = self.shell('source "$2/parse-error.zsh" 2>/dev/null; [[ $? != 0 ]] || exit 93; zpmod source-study -l >/dev/null; zpmod source-study --json 0')
        self.assertEqual(report["status"], "complete")
        self.assertEqual(len(report["events"]), 1)
        self.assertEqual(report["events"][0]["source_status"], 2)
        self.assertEqual(report["events"][0]["exit_status"], 126)

    def test_count_and_cwd_identity(self):
        report = self.shell('source "$3"; cd "$2"; source ./change-dir.zsh; zpmod source-study --json 1')
        self.assertEqual(self.paths(report), [os.fsencode(self.root / "change-dir.zsh")])
        self.assertEqual(report["events"][0]["id"], 2)

    def test_non_utf8_path_is_lossless(self):
        path = os.fsencode(self.root) + b"/non-utf8-\xff.zsh"
        with open(path, "wb") as stream:
            stream.write(b":\n")
        report = self.shell('for f in "$2"/non-utf8-*; do source "$f"; done; zpmod source-study --json')
        self.assertEqual(self.paths(report), [path])

    def test_installed_package_json(self):
        packages = self.root / "packages"
        packages.mkdir()
        subprocess.run(["cpack", "--config", str(Path(args.build_dir) / "CPackConfig.cmake"), "-G", "TGZ", "-C", "Release", "-B", str(packages)], check=True, capture_output=True)
        archive, = packages.glob("*.tar.gz")
        extracted = self.root / "extracted"
        extracted.mkdir()
        subprocess.run(["tar", "-xzf", str(archive), "-C", str(extracted)], check=True, capture_output=True)
        module, = [p for p in extracted.rglob("zpmod.*") if p.parent.name == "site-modules"]
        report = self.shell('source "$3"; zpmod source-study --json', module.parent)
        self.assertEqual(self.paths(report), [os.fsencode(self.child)])
        self.assertEqual(report["status"], "complete")


if __name__ == "__main__":
    unittest.main(argv=[__file__, *remaining])
