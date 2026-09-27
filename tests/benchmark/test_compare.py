#!/usr/bin/env python3
"""Check that invalid evidence fails and timing changes remain review flags."""
import copy
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("comparison", Path(__file__).parents[2] / "benchmarks/compare.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def fixture():
    record = {"source_revision": "a" * 40, "environment": {
        "zsh_version": "5.9.2", "architecture": "x86_64", "cpu": "fixture",
        "module_sha256": "b" * 64}, "workload": {"warmups": 5, "scripts": 40, "functions_per_script": 30},
        "cases": [{"id": "plain", "samples_ms": [10.0]}]}
    return {label: [copy.deepcopy(record) for _ in range(3)] for label in module.VARIANTS}


class EvidenceTests(unittest.TestCase):
    def test_slowdown_flags_without_functional_failure(self):
        records = fixture()
        for record in records["candidate"]:
            record["cases"][0]["samples_ms"] = [20.0]
        report = module.comparison(records, "sha256:" + "c" * 64, 3)
        self.assertEqual(report["flagged"], ["plain"])
        self.assertEqual(report["failed"], [])

    def test_runtime_mismatch_invalidates_comparison(self):
        records = fixture()
        for record in records["candidate"]:
            record["environment"]["zsh_version"] = "5.8.1"
        with self.assertRaisesRegex(ValueError, "incompatible"):
            module.comparison(records, "fixture", 3)

    def test_control_must_be_same_baseline_module(self):
        records = fixture()
        for record in records["control"]:
            record["environment"]["module_sha256"] = "d" * 64
        with self.assertRaisesRegex(ValueError, "A/A"):
            module.comparison(records, "fixture", 3)

    def test_changed_fixture_invalidates_comparison(self):
        records = fixture()
        for record in records["candidate"]:
            record["workload"]["scripts"] = 4
        with self.assertRaisesRegex(ValueError, "incompatible"):
            module.comparison(records, "fixture", 3)

    def test_nonfinite_durations_rejected(self):
        for value in [float("nan"), float("inf"), -1, True]:
            with self.assertRaises(ValueError):
                module.summary([value])

    def test_zero_baseline_has_no_percentage(self):
        result = module.row(module.summary([0, 0, 0]), module.summary([1, 1, 1]))
        self.assertIsNone(result["change"]["median_delta_percent"])

    def test_every_position_rotates_in_first_three_rounds(self):
        for position in range(3):
            self.assertEqual({order[position] for order in module.ORDER[:3]}, set(module.VARIANTS))


if __name__ == "__main__":
    unittest.main()
