#include "project.h"

/* A client table that tracks the client connection status */
uint8_t client_table[MAX_CLIENT_NUM];

/* A client queue array for connected clients */
x_queue_t client_queue[MAX_CLIENT_NUM];


int main(int argc, char const *argv[])
{
    pthread_t task_serial;

  /* Create all other threads */
  if(pthread_create(&task_serial, NULL, task_Serial, NULL) != 0)
  {
      perror("[ERROR] [MAIN] Failed to create the serial task.\n");
  }
  DEBUG(("[Main_Task] Created the serial task.\n"));
  
  /* Perform local heartbeat check */

  /* Perform client heartbeat check */

  pthread_join(task_serial, NULL);
  return 0;
}
