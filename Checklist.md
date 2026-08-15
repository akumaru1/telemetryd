# Project Setup & Execution Checklist: Linux Telemetry Daemon in C

---

## Phase 1: Environment & Project Scaffolding
- [x] **Directory Structure**: Set up clean folder layout: | [Plan](docs/plans/phase_one_plan.md) | [Walkthrough](docs/walkthroughs/phase_one_walkthrough.md)
  ```text
  telemetry-daemon/
  ├── include/       # Header files (.h)
  ├── src/           # Source files (.c)
  ├── config/        # daemon.conf
  ├── docs/          # Doxygen output directory
  ├── tests/         # Unit test files
  ├── Makefile       # Build automation
  └── CHECKLIST.md
  ```

- [x] Tooling Check: Verify local installation of required tools: | [Plan](docs/plans/phase_one_plan.md) | [Walkthrough](docs/walkthroughs/phase_one_walkthrough.md)

        GCC / Clang compiler (gcc --version)

        Build automation (make --version)

        Memory analysis tool (valgrind --version)

        Documentation generator (doxygen --version)

---

Phase 2: Core Hardware & Kernel Reading (C Logic)

- [x] Sysfs Ingestion: Implement opening and reading CPU thermal metrics from /sys/class/hwmon/hwmon0/temp1_input using standard C File I/O (fopen, fscanf, fclose). | [Plan](docs/plans/sysfs_ingestion_plan.md) | [Walkthrough](docs/walkthroughs/sysfs_ingestion_walkthrough.md)
- [x] Procfs Ingestion: Implement parsing /proc/meminfo or /proc/stat to capture memory usage or system activity. | [Plan](docs/plans/procfs_ingestion_plan.md) | [Walkthrough](docs/walkthroughs/procfs_ingestion_walkthrough.md)

- [x] Data Struct Definition: Define a clean telemetry_sample_t struct containing timestamp, CPU temp, and RAM utilization. | [Plan](docs/plans/data_struct_definition_plan.md) | [Walkthrough](docs/walkthroughs/data_struct_definition_walkthrough.md)

---

Phase 3: Advanced Data Structures & POSIX Features

- [ ] Ring Buffer Implementation:

    - [x] Implement fixed-size circular array struct (ring_buffer_t). | [Plan](docs/plans/ring_buffer_plan.md) | [Walkthrough](docs/walkthroughs/ring_buffer_walkthrough.md)

    - [x] Write thread-safe / clean push() and pop() operations. | [Plan](docs/plans/ring_buffer_concurrency_plan.md) | [Walkthrough](docs/walkthroughs/ring_buffer_concurrency_walkthrough.md)
          - Implementing the circular buffer (ring_buffer_t) with mutex lock protection satisfies the "thread-safe circular ring buffer"

    - [x] Add logic to flush buffer entries to a structured log file (e.g., telemetry.log) when full or on time intervals. | [Plan](docs/plans/daemon_integration_plan.md) | [Walkthrough](docs/walkthroughs/daemon_integration_walkthrough.md)

- [ ] POSIX Signal Handling:

    - [x] Register SIGINT and SIGTERM using sigaction() to set a volatile sig_atomic_t keep_running flag to 0. | [Plan](docs/plans/signal_handling_plan.md) | [Walkthrough](docs/walkthroughs/signal_handling_walkthrough.md)

    - [x] Ensure all file descriptors are closed and allocated memory is freed during shutdown. | [Plan](docs/plans/daemon_integration_plan.md) | [Walkthrough](docs/walkthroughs/daemon_integration_walkthrough.md)

    - [ ] Dynamic Re-configuration (SIGHUP):

    - [ ] Register SIGHUP signal handler.

    - [ ] Write a helper function to re-parse config/daemon.conf (e.g., updating polling frequency or log paths on the fly).

- Registering sigaction() for SIGINT/SIGTERM (to set keep_running = 0 for clean shutdown) and SIGHUP (to trigger config re-parsing) directly matches the "graceful shutdown and runtime config reloading".

---

Phase 4: Quality Assurance & Verification

- [ ] Zero Memory Leaks Verification:

    - [ ] Compile with debug flags: gcc -Wall -Wextra -g -Iinclude src/*.c -o daemon

    - [ ] Run under Valgrind: valgrind --leak-check=full --show-leak-kinds=all ./daemon

    - [ ] Trigger Ctrl+C (SIGINT) and verify "All heap blocks were freed -- no leaks are possible".

- [ ] Compilation Cleanliness: Build with -Wall -Wextra -pedantic and resolve all compiler warnings.

---

Phase 5: Production Deployment & System Integration

- [ ] Systemd Unit File:

    - [ ] Write /etc/systemd/system/telemetry-daemon.service.

    - [ ] Define ExecStart, Restart=on-failure, and StandardOutput=journal.

    - [ ] Daemon Testing:

    - [ ] Load and enable service: sudo systemctl daemon-reload && sudo systemctl start telemetry-daemon.

    - [ ] Check service state: systemctl status telemetry-daemon.

    - [ ] Verify system logs: journalctl -u telemetry-daemon -f.

---

Phase 6: Documentation & Doxygen

- [ ] Code Annotation: Add Doxygen comments (/** ... */) above function headers in include/ using @brief, @param, and @return tags.

- [ ] Generate Docs:

    - [ ] Generate default configuration: doxygen -g Doxyfile

    - [ ] Run doxygen Doxyfile and confirm docs/html/index.html builds correctly.

---

Phase 7: Resume & Version Control Finalization

- [ ] Git Repository: Initialize Git, write clean commit messages, and push to GitHub.

- [ ] Update Resume: Add bullet points highlighting C, POSIX signals, sysfs/procfs, ring buffer, systemd, and Doxygen.