#include <stdio.h>
#include <signal.h>

#include "handler.h"

extern volatile sig_atomic_t signals_arrived;

void signal_handler()
{
    signals_arrived++;
}
