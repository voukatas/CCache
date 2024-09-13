#include "../include/signal_handler.h"

// volatile sig_atomic_t keep_running = 1;
atomic_int keep_running = 1;

void set_event_loop_state(int state) {  // 0 for stop, 1 for running
    atomic_store(&keep_running, state);
}

void handle_shutdown_signal(int signal_number) {  // 0 for stop, 1 for running
    (void)signal_number;
    atomic_store(&keep_running, 0);
}

void setup_signal_handling(void) {
    struct sigaction sa;
    sa.sa_handler = handle_shutdown_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
