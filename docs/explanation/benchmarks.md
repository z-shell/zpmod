# Benchmark Methodology

zpmod's public benchmark compares one generated, parse-heavy workload across plain sourcing, zpmod's first run, zpmod's warm path, and
manually compiled `.zwc` files. The manual control prevents Zsh bytecode performance from being misattributed to zpmod itself.

The harness records every sample plus the median, p95, standard deviation, module hash, source revision, Zsh version, operating system,
architecture, CPU, workload size, and sampling counts. Cases run in a balanced rotating order with isolated `HOME` and `ZDOTDIR`
directories.

"First run" means no `.zwc` exists before the measured shell starts. It does not claim that operating-system filesystem caches were cleared.

See the [benchmark runner, limitations, and published evidence](../../benchmarks/README.md) for reproduction commands and raw result links.
