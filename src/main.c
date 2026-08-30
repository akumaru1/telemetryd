#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "sysfs_ingest.h"
#include "procfs_ingest.h"
#include "ring_buffer.h"

/* Compiled-in configuration */
#define POLLING_FREQUENCY 5
#define LOG_PATH "telemetry.log"

/* Volatile sig_atomic_t flag for signal handling */
volatile sig_atomic_t keep_running = 1;

/* Ring buffer for telemetry samples */
static ring_buffer_t rb;

/* Log file descriptor */
static FILE *log_file = NULL;
static int console_mode = 0;

static void close_log_file(void) {
  if (log_file && log_file != stdout && log_file != stderr) {
    fclose(log_file);
    log_file = NULL;
  }
}

/* Signal handler for SIGINT and SIGTERM */
static void handle_signal(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    keep_running = 0;
  }
}

static void *collector_thread_func(void *arg);
static void *writer_thread_func(void *arg);

int main(int argc, char *argv[]) {
  // Check for console mode flag
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--console") == 0) {
      console_mode = 1;
    }
  }

  // Initialize ring buffer
  int rb_rc = ring_buffer_init(&rb, 64);
  if (rb_rc != 0) {
    fprintf(stderr, "Failed to initialize ring buffer: %s [%d]\n",
            ring_buffer_strerror(rb_rc), rb_rc);
    return 1;
  }

  // Open log destination
  if (console_mode) {
    log_file = stdout;
    printf("Running in console mode. Outputting directly to stdout.\n");
  } else {
    log_file = fopen(LOG_PATH, "a");
    if (!log_file) {
      perror("Failed to open log file");
      ring_buffer_destroy(&rb);
      return 1;
    }
  }

  // Set up sigaction structure
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  // Register handlers
  if (sigaction(SIGINT, &sa, NULL) < 0) {
    perror("Error registering SIGINT handler");
    close_log_file();
    ring_buffer_destroy(&rb);
    return 1;
  }
  if (sigaction(SIGTERM, &sa, NULL) < 0) {
    perror("Error registering SIGTERM handler");
    close_log_file();
    ring_buffer_destroy(&rb);
    return 1;
  }

  printf("Telemetry daemon started. PID: %d. Press Ctrl+C or kill to stop...\n",
          getpid());


  // Start worker threads
  pthread_t collector_thread;
  pthread_t writer_thread;

  if (pthread_create(&collector_thread, NULL, collector_thread_func, NULL) != 0) {
    fprintf(stderr, "Error creating collector thread.\n");
    close_log_file();
    ring_buffer_destroy(&rb);
    return 1;
  }
  if (pthread_create(&writer_thread, NULL, writer_thread_func, NULL) != 0) {
    fprintf(stderr, "Error creating writer thread.\n");
    keep_running = 0; // stop collector
    pthread_join(collector_thread, NULL);
    close_log_file();
    ring_buffer_destroy(&rb);
    return 1;
  }

  // Main thread waits for shutdown signal
  while (keep_running) {
    sleep(1);
  }

  printf("Shutdown signal received. Starting graceful cleanup...\n");

  // Join threads
  pthread_join(collector_thread, NULL);
  pthread_join(writer_thread, NULL);

  // Close log file
  close_log_file();

  // Clean up ring buffer and mutexes/condition variables
  ring_buffer_destroy(&rb);

  printf("All threads joined, file descriptors closed, and memory freed successfully. Exiting.\n");
  return 0;
}

/* Metrics collector thread routine */
static void *collector_thread_func(void *arg) {
  (void)arg;
  printf("Collector thread started.\n");

  while (keep_running) {
    telemetry_sample_t sample;
    sample.timestamp = time(NULL);

    int res_cpu = read_cpu_temp(&sample.cpu_temp);
    int res_ram = read_ram_utilization(&sample.ram_utilization);

    if (res_cpu == 0 && res_ram == 0) {
      int push_res = ring_buffer_push(&rb, sample);
      if (push_res != 0) {
        fprintf(stderr, "Collector warning: failed to push sample to ring buffer: %s [%d]\n",
                ring_buffer_strerror(push_res), push_res);
      }
    } else {
      fprintf(stderr, "Collector error: failed to read metrics (CPU: %s [%d], RAM: %s [%d])\n",
              sysfs_ingest_strerror(res_cpu), res_cpu,
              procfs_ingest_strerror(res_ram), res_ram);
    }

    // Sleep in 1-second ticks so we can exit quickly on signal
    for (int i = 0; i < POLLING_FREQUENCY && keep_running; i++) {
      sleep(1);
    }
  }

  printf("Collector thread exiting...\n");
  return NULL;
}

/* Log writer thread routine */
static void *writer_thread_func(void *arg) {
  (void)arg;
  printf("Writer thread started.\n");

  while (keep_running) {
    telemetry_sample_t sample;
    int popped_any = 0;

    // Pop all currently available elements
    while (ring_buffer_pop(&rb, &sample) == 0) {
      if (log_file) {
        fprintf(log_file, "[%ld] CPU Temp: %.3f C, RAM Util: %.3f%%\n",
                (long)sample.timestamp, sample.cpu_temp, sample.ram_utilization);
        popped_any = 1;
      }
    }

    if (popped_any && log_file) {
      fflush(log_file);
    }

    // Process every 1 second
    sleep(1);
  }

  // Drain the ring buffer one final time during shutdown
  printf("Writer thread draining ring buffer...\n");
  telemetry_sample_t sample;
  int popped_any = 0;

  while (ring_buffer_pop(&rb, &sample) == 0) {
    if (log_file) {
      fprintf(log_file, "[%ld] CPU Temp: %.3f C, RAM Util: %.3f%%\n",
              (long)sample.timestamp, sample.cpu_temp, sample.ram_utilization);
      popped_any = 1;
    }
  }

  if (popped_any && log_file) {
    fflush(log_file);
  }

  printf("Writer thread exiting...\n");
  return NULL;
}
