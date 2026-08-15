#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Volatile sig_atomic_t flag for graceful shutdown */
volatile sig_atomic_t keep_running = 1;

/* Signal handler for SIGINT and SIGTERM */
static void handle_signal(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    keep_running = 0;
  }
}

int main(void) {
  // Set up sigaction structure
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  // Register handler for SIGINT
  if (sigaction(SIGINT, &sa, NULL) < 0) {
    perror("Error registering SIGINT handler");
    return 1;
  }

  // Register handler for SIGTERM
  if (sigaction(SIGTERM, &sa, NULL) < 0) {
    perror("Error registering SIGTERM handler");
    return 1;
  }

  printf("Telemetry daemon started. PID: %d. Press Ctrl+C or kill to stop...\n",
         getpid());

  // Main execution loop
  while (keep_running) {
    printf("Daemon is running...\n");
    sleep(1);
  }

  printf("Received shutdown signal. Exiting...\n");
  return 0;
}
