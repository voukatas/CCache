#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>
#include <stdlib.h>
#include <stdatomic.h>

#ifdef TESTING
//#warning "TESTING macro is defined"
    #define KEEP_RUNNING_TYPE atomic_int
    #define KEEP_RUNNING_INIT 1
    #define KEEP_RUNNING_STORE(var, val) atomic_store(&(var), (val))
    #define KEEP_RUNNING_LOAD(var) atomic_load(&(var))
#else
    #define KEEP_RUNNING_TYPE volatile sig_atomic_t
    #define KEEP_RUNNING_INIT 1
    #define KEEP_RUNNING_STORE(var, val) (var = (val))
    #define KEEP_RUNNING_LOAD(var) (var)
#endif

void set_event_loop_state(int state);
void handle_shutdown_signal(int signal_number);
void setup_signal_handling(void);
extern KEEP_RUNNING_TYPE keep_running;

#endif // SIGNAL_HANDLER_H
