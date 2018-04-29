#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <poll.h>
#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "task.h"
#include "messageConfig.h"
#include "message.h"
#include "util.h"

#define DEBUG_ON (1)

#if DEBUG_ON == 1
    #define DEBUG(a) printf a
#else
    #define DEBUG(a) (void)0
#endif

#define OPEN_MAX    (10)
#define MAX_CLIENT_NUM  (4)

#define SERVER_PORT (9999)

extern sem_t mr_sem, tx_sem, lg_sem, cm_sem;
extern sem_t mr_hb_sem, tx_hb_sem, rx_hb_sem, lg_hb_sem;

extern struct pollfd client[OPEN_MAX];
extern x_queue_t router_q, logger_q;

extern msg_t txbuf;

extern msg_t response[2];

#endif
