# Profiling Design

The source evaluator records each completed source attempt with an increasing ID, nesting depth, lexical absolute path, source and command
statuses, and an integer elapsed duration. It snapshots paths before execution and retains repeated source events. PATH probes rejected
before evaluation are not recorded.

The default clock is monotonic where the platform supports it, with an explicitly identified `gettimeofday` fallback. Duration arithmetic
preserves sub-millisecond values, detects backwards intervals and overflow, and marks unavailable samples instead of inventing zero
durations. Nanosecond units do not imply nanosecond clock resolution.

Timings include nested sourcing and the module's compilation behavior. Parent and child durations overlap, so adding every event does not
measure startup wall time. Instrumented runs also differ from ordinary execution because zpmod may create or use `.zwc` files.

`zpmod source-study` retains the human-readable whole-millisecond table; `-l` selects full paths. `--json` emits the
[versioned source-study contract](../reference/cli.md#json-schema-1), including capture completeness, clock provenance and lossless path
identities. Reporting does not clear history. Formatting is deferred until reporting.

Use this output to locate expensive source calls. Reproducible performance comparisons still need an external harness controlling workload,
environment, warmups, repeated samples and an A/A control. This profiler is not a universal benchmark dependency or a CI slowdown gate.
