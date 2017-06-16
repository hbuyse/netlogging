#include <stdio.h>
#include <execinfo.h> // backtrace, backtrace_symbols
#include <unistd.h> // getpid
#include <signal.h> // sigaction, sigemptyset, sigaction
#include <string.h> // strcpy
#include <stdlib.h> // free

#include "netlogging.h"

#define PORT 65432




/*============================================================================*/
/* dump_backtrace                                                             */
/*============================================================================*/
static void dump_backtrace(int signal, void *foo)
{
    void *array[32];
    size_t size;
    char **strings;
    size_t i;
    struct sigaction sa;

    size = backtrace (array, 32);
    strings = backtrace_symbols (array, size);

    char process[128];
    sprintf(process, "%d", getpid());

    fprintf(stderr, ">>> SEGMENTATION FAULT ON PROCESS %s >>>\n", process);

    for (i = 0; i < size; i++)
    {
        fprintf(stderr, "%s\n", strings[i]);
    }

    fprintf(stderr, "<<< SEGMENTATION FAULT ON PROCESS %s <<<", process);

    free (strings);

    /* Pass on the signal (so that a core file is produced).  */
    sa.sa_handler = SIG_DFL;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction (signal, &sa, NULL);
    raise (signal);
}

int main(int        argc,
         char const *argv[]
         )
{
    struct sigaction sa;

    // Install the SEGFAULT handler
    sa.sa_handler = (void *) dump_backtrace;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);

    netlogg_init(argv[0], PORT, NETLOGG_DEBUG);

    netlogg_start();

    return (0);
}