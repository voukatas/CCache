#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>
#include <stdlib.h>
#include <stdatomic.h>

void set_event_loop_state(int state);
void handle_shutdown_signal(int signal_number);
void setup_signal_handling(void);
extern atomic_int keep_running;

#endif // SIGNAL_HANDLER_H
