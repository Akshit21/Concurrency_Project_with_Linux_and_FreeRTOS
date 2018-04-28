#include "message.h"
#include <stdio.h>
#include <stdint.h>
#include <time.h>


void getTimestamp(char * buf)
{
    time_t t = time(NULL);
    sprintf(buf, "%ju", (uintmax_t)t);
}
