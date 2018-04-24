#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <semaphore.h>

#include "task.h"
#include "messageConfig.h"
#include "message.h"

#define DEBUG (1)

#if DEBUG_ON == 1
    #define DEBUG(a) printf a
#else
    #define DEBUG(a) (void)0
#endif

#define MAX_CLIENT_NUM  (10)

extern uint8_t client_table[MAX_CLIENT_NUM];
extern x_queue_t client_queue[MAX_CLIENT_NUM];

extern sem_t ser_hb_sem, ser_req_sem;

#endif
