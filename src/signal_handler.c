#include "../include/signal_handler.h"

KEEP_RUNNING_TYPE keep_running = KEEP_RUNNING_INIT;
// KEEP_RUNNING_STORE(keep_running, KEEP_RUNNING_INIT);

void set_event_loop_state(int state) {  // 0 for stop, 1 for running
                                        // #warning "TESTING macro is defined"
    KEEP_RUNNING_STORE(keep_running, state);
}

void handle_shutdown_signal(int signal_number) {  // 0 for stop, 1 for running
    (void)signal_number;
    KEEP_RUNNING_STORE(keep_running, 0);
}

void setup_signal_handling(void) {
    struct sigaction sa;
    sa.sa_handler = handle_shutdown_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
