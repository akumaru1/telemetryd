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

#define CONFIG_PATH "config/daemon.conf"
#define DEFAULT_POLLING_FREQUENCY 5
#define DEFAULT_LOG_PATH "telemetry.log"

/* Volatile sig_atomic_t flags for signal handling */
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t reload_config = 0;

/* Ring buffer for telemetry samples */
static ring_buffer_t rb;

/* Log file descriptor and protecting mutex */
static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static int console_mode = 0;

static void close_log_file(void) {
  if (log_file && log_file != stdout && log_file != stderr) {
    fclose(log_file);
    log_file = NULL;
  }
}

/* Daemon configuration variables and protecting mutex */
static int polling_frequency = DEFAULT_POLLING_FREQUENCY;
static char log_path[256] = DEFAULT_LOG_PATH;
static pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Signal handler for SIGINT, SIGTERM, and SIGHUP */
static void handle_signal(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    keep_running = 0;
  } else if (sig == SIGHUP) {
    reload_config = 1;
  }
}

/* Minimal parser for daemon.conf */
static int parse_config(const char *config_path, int *polling_frequency_out, char *log_path_out, size_t max_log_path_len) {
  FILE *fp = fopen(config_path, "r");
  if (!fp) {
    return -1;
  }

  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    // Skip comments and empty lines
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
      continue;
    }

    char key[128];
    char val[128];
    if (sscanf(line, " %127[^= ] = %127s", key, val) == 2) {
      if (strcmp(key, "polling_frequency") == 0) {
        *polling_frequency_out = atoi(val);
      } else if (strcmp(key, "log_path") == 0) {
        strncpy(log_path_out, val, max_log_path_len - 1);
        log_path_out[max_log_path_len - 1] = '\0';
      }
    }
  }

  fclose(fp);
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
        fprintf(stderr, "Collector warning: failed to push sample to ring buffer: %d\n", push_res);
      }
    } else {
      fprintf(stderr, "Collector error: failed to read metrics (CPU: %s [%d], RAM: %s [%d])\n",
              sysfs_ingest_strerror(res_cpu), res_cpu,
              procfs_ingest_strerror(res_ram), res_ram);
    }

    // Get current polling frequency thread-safely
    pthread_mutex_lock(&config_mutex);
    int current_freq = polling_frequency;
    pthread_mutex_unlock(&config_mutex);

    // Sleep in 1-second ticks so we can exit quickly on signal or config reload
    for (int i = 0; i < current_freq && keep_running && !reload_config; i++) {
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
      pthread_mutex_lock(&log_mutex);
      if (log_file) {
        fprintf(log_file, "[%ld] CPU Temp: %.3f C, RAM Util: %.3f%%\n",
                (long)sample.timestamp, sample.cpu_temp, sample.ram_utilization);
        popped_any = 1;
      }
      pthread_mutex_unlock(&log_mutex);
    }

    if (popped_any) {
      pthread_mutex_lock(&log_mutex);
      if (log_file) {
        fflush(log_file);
      }
      pthread_mutex_unlock(&log_mutex);
    }

    // Process every 1 second
    sleep(1);
  }

  // Drain the ring buffer one final time during shutdown
  printf("Writer thread draining ring buffer...\n");
  telemetry_sample_t sample;
  int popped_any = 0;

  while (ring_buffer_pop(&rb, &sample) == 0) {
    pthread_mutex_lock(&log_mutex);
    if (log_file) {
      fprintf(log_file, "[%ld] CPU Temp: %.3f C, RAM Util: %.3f%%\n",
              (long)sample.timestamp, sample.cpu_temp, sample.ram_utilization);
      popped_any = 1;
    }
    pthread_mutex_unlock(&log_mutex);
  }

  if (popped_any) {
    pthread_mutex_lock(&log_mutex);
    if (log_file) {
      fflush(log_file);
    }
    pthread_mutex_unlock(&log_mutex);
  }

  printf("Writer thread exiting...\n");
  return NULL;
}

int main(int argc, char *argv[]) {
  // Check for console mode flag
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--console") == 0) {
      console_mode = 1;
    }
  }

  // Load initial configuration
  if (parse_config(CONFIG_PATH, &polling_frequency, log_path, sizeof(log_path)) != 0) {
    printf("Could not parse config, using defaults: polling_freq = %d, log_path = %s\n",
           polling_frequency, log_path);
  } else {
    printf("Loaded config: polling_freq = %d, log_path = %s\n",
           polling_frequency, log_path);
  }

  // Initialize ring buffer
  if (ring_buffer_init(&rb, 64) != 0) {
    fprintf(stderr, "Failed to initialize ring buffer.\n");
    return 1;
  }

  // Open log destination
  if (console_mode) {
    log_file = stdout;
    printf("Running in console mode. Outputting directly to stdout.\n");
  } else {
    log_file = fopen(log_path, "a");
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

  if (sigaction(SIGHUP, &sa, NULL) < 0) {
    perror("Error registering SIGHUP handler");
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

  // Main thread waits for shutdown or reload signal
  while (keep_running) {
    if (reload_config) {
      reload_config = 0;
      printf("SIGHUP received. Reloading configuration...\n");

      int new_freq = DEFAULT_POLLING_FREQUENCY;
      char new_path[256] = DEFAULT_LOG_PATH;

      if (parse_config(CONFIG_PATH, &new_freq, new_path, sizeof(new_path)) == 0) {
        pthread_mutex_lock(&config_mutex);
        polling_frequency = new_freq;

        if (!console_mode && strcmp(log_path, new_path) != 0) {
          printf("Log path changed from '%s' to '%s'. Reopening log file...\n", log_path, new_path);
          strncpy(log_path, new_path, sizeof(log_path) - 1);
          log_path[sizeof(log_path) - 1] = '\0';

          pthread_mutex_lock(&log_mutex);
          close_log_file();
          log_file = fopen(log_path, "a");
          if (!log_file) {
            perror("Failed to open new log file");
          }
          pthread_mutex_unlock(&log_mutex);
        }
        pthread_mutex_unlock(&config_mutex);
        printf("Configuration reloaded successfully. Frequency: %d, Log Path: %s\n", new_freq, new_path);
      } else {
        fprintf(stderr, "Failed to parse config file on SIGHUP. Keeping existing configuration.\n");
      }
    }
    sleep(1);
  }

  printf("Shutdown signal received. Starting graceful cleanup...\n");

  // Join threads
  pthread_join(collector_thread, NULL);
  pthread_join(writer_thread, NULL);

  // Close log file
  pthread_mutex_lock(&log_mutex);
  close_log_file();
  pthread_mutex_unlock(&log_mutex);

  // Clean up ring buffer and mutexes/condition variables
  ring_buffer_destroy(&rb);

  printf("All threads joined, file descriptors closed, and memory freed successfully. Exiting.\n");
  return 0;
}
