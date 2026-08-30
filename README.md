# telemetryd (`Telemetry Daemon`)

A lightweight Linux system telemetry daemon written in C. `telemetryd` continuously monitors system health by reading CPU thermal metrics from `sysfs` and RAM utilization from `procfs`. 

To prevent disk I/O latency from affecting metric collection intervals, the daemon utilizes a multi-threaded architecture with a lock-protected, thread-safe circular ring buffer.

---

## Features

- **Core Ingestion:**
  - **CPU Temperature:** Reads CPU temperature directly from driver-specific `sysfs` paths (AMD `k10temp` or Intel `coretemp`).
  - **RAM Utilization:** Parses `/proc/meminfo` to calculate instantaneous memory usage.
- **Concurrency:** Decouples collection and writing using a thread-safe circular ring buffer protected by mutexes and condition variables.
- **POSIX Signal Handling:**
  - `SIGINT` / `SIGTERM` signals trigger a graceful shutdown (draining remaining metrics, joining threads and freeing all resources).

---

## Directory Structure

```text
telemetryd/
├── bin/           # Output directory for compiled binaries
├── include/       # Header files (.h)
├── obj/           # Intermediate object files (.o)
├── src/           # Daemon source code files (.c)
├── tests/         # Unit testing suite
└── Makefile       # Build automation script
```

---

## Getting Started

### Prerequisites

Ensure the following tools are installed on your Linux system:
- GCC compiler
- Make build automation utility
- Valgrind (optional, for memory analysis verification)

### 1. Building the Project

Compile the main daemon executable using `make`:
```bash
make
```
This produces the binary at `./bin/telemetryd`.

To clean intermediate object files and build binaries:
```bash
make clean
```

### 2. Configuration

Configuration is compiled in. Adjust `POLLING_FREQUENCY` (seconds between metrics
snapshots) and `LOG_PATH` (telemetry log file) at the top of `src/main.c` and
rebuild. Defaults: `5` seconds, `telemetry.log`.

---

## Running the Daemon

### Console Mode (Interactive Mode)
If you want to view metrics printed directly to your terminal screen in real time:
```bash
./bin/telemetryd -c
```

**Example Output:**
```text
Running in console mode. Outputting directly to stdout.
Telemetry daemon started. PID: 402621. Press Ctrl+C or kill to stop...
Collector thread started.
Writer thread started.
[1786806991] CPU Temp: 50.750 C, RAM Util: 63.499%
[1786806996] CPU Temp: 50.750 C, RAM Util: 63.518%
```

*Press `Ctrl+C` at any time to shut down the daemon cleanly.*
---

## Runtime Operations

### Graceful Shutdown
To cleanly terminate the daemon process:
```bash
kill -INT <PID>    # equivalent to pressing Ctrl+C
```
The daemon will drain the ring buffer to the log file, join both worker threads, close file descriptors and exit cleanly with zero memory leaks.

---

## Testing & Verification

### Running the Unit Tests
Compile and run test suites for the sysfs sensor reader, procfs parser, data structures and ring buffer concurrency:
```bash
make test
```

### Memory Leak Verification (Valgrind)
To verify that the daemon achieves perfect memory management during standard operations and graceful shutdown:
1. Run the daemon under Valgrind:
   ```bash
   valgrind --leak-check=full --show-leak-kinds=all ./bin/telemetryd
   ```
2. Trigger a graceful exit by sending a termination signal (e.g., `Ctrl+C` in console mode or `kill -INT`).
3. Verify that the Valgrind report displays:
   `All heap blocks were freed -- no leaks are possible`.
