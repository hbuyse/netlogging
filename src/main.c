#include <stdio.h>

#include "netlogging.h"

#define PORT 65432

int main(int        argc,
         char const *argv[]
         )
{
    netlogg_init(argv[0], PORT, NETLOGG_DEBUG);

    netlogg_start();

    return (0);
}