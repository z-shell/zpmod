# Profiling Design

## Objectives

- Capture every sourced path early (even before plugin manager logic).
- Provide user-friendly timing table with minimal overhead.

## Data Model

Hashtable `zp_source_events` entries contain:

- Incrementing ID
- Start timestamp (ms)
- Directory path
- File name
- Full path
- Duration (ms)
- Load result (success / error code)

## Timing Method

`gettimeofday()` at entry and exit; difference stored as floating milliseconds.

## Reporting

`zpmod source-study` builds a string buffer sized adaptively. `-l` toggles full vs basename output.

## Overhead Considerations

- Allocation minimized by reusing stack buffers where possible.
- Only simple arithmetic performed in the hot path; formatting deferred to report generation.

## Future Enhancements (Potential)

- Aggregated totals per directory / plugin.
- Percent contribution of each script to total startup time.
- Threshold filtering (show entries above N ms).
