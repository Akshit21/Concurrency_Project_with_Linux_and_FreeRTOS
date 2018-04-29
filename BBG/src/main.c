#include "project.h"

sem_t           mr_sem, tx_sem, lg_sem, cm_sem;
sem_t           mr_hb_sem, tx_hb_sem, rx_hb_sem, lg_hb_sem;

/* Message queues for the MsgRouter and Logger task */
x_queue_t       router_q, logger_q;

/* A client table that tracks the client connection status */
struct pollfd   client[OPEN_MAX];

int main(int argc, char const *argv[])
{
    pthread_t msgrouter, tx, rx, logger, command;

    /* Initialize semaphores */
    if((sem_init(&mr_sem,0,0)!=0) || (sem_init(&tx_sem,0,0)!=0) ||
       (sem_init(&lg_sem,0,0)!=0) || (sem_init(&cm_sem,0,0)!=0))
    {
        perror("[ERROR] [main] sem_init() failed.\n");
    }

    /* Create queues */
    if((msg_create_LINUX_mq("/router", 10, &router_q)!=0) ||
       (msg_create_LINUX_mq("/logger", 10, &logger_q)!=0))
    {
        perror("[ERROR] [main] Failed to initialize queues.\n");
    }

    /* Create all other threads */
    if((pthread_create(&rx, NULL, RX, NULL) != 0) ||
       (pthread_create(&tx, NULL, task_Tx, NULL) != 0)             ||
       (pthread_create(&logger, NULL, task_Logger, NULL) != 0)     ||
       (pthread_create(&command, NULL, task_Command, NULL) != 0)   ||
       (pthread_create(&msgrouter, NULL, task_MsgRouter, NULL) != 0))
    {
        perror("[ERROR] [MAIN] Failed to create tasks.\n");
    }


  /* Perform local heartbeat check */

  /* Perform client heartbeat check */

  pthread_join(rx,  NULL);
  pthread_join(tx,        NULL);
  pthread_join(logger,    NULL);
  pthread_join(command,   NULL);
  pthread_join(msgrouter, NULL);
  return 0;
}
