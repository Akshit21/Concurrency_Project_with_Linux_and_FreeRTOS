#include "project.h"

void * task_Logger(void * param)
{
    FILE            *pfile;
    char            log[200], client_name[MAX_CLIENT_ID_LENGTH+1];
    msg_t           log_msg;
    struct timespec wait_time;

    pfile = fopen("serverlog.txt", "w");
    fclose(pfile);

    for( ; ; )
    {
        if (clock_gettime(CLOCK_REALTIME, &wait_time) == -1)
        {
            perror("clock_gettime\n");
        }
        wait_time.tv_sec += 1;
        if(sem_timedwait(&lg_sem, &wait_time)==0)
        {
            /* Logs pending */
            if(msg_receive_LINUX_mq(&logger_q, &log_msg)!=0)
            {
                perror("[ERROR] [task_Logger] Failed to dequeue log.\n");
            }
            else
            {
                if(log_msg.id == 0)
                {
                    /* Server log */
                    if((pfile = fopen("serverlog.txt", "a+"))==NULL)
                    {
                        perror("[ERROR] [task_Logger] Server log, fopen() failed.\n");
                    }
                    sprintf(log, "[%s] %s", log_msg.timestamp, log_msg.content);
                    if(fwrite(log, strlen(log), 1, pfile)<=0)
                    {
                        perror("[ERROR] [task_Logger] Server log, fwrite() failed.\n");
                    }
                    fclose(pfile);
                }
                else
                {
                    /* Client log */
                    sprintf(client_name, "%d.txt", log_msg.id);
                    if((pfile = fopen(client_name, "a+"))==NULL)
                    {
                        perror("[ERROR] [task_Logger] Client log, fopen() failed.\n");
                    }
                    sprintf(log, "[%s] %s", log_msg.timestamp, log_msg.content);
                    if(fwrite(log, strlen(log), 1, pfile)<=0)
                    {
                        perror("[ERROR] [task_Logger] Client log, fwrite() failed.\n");
                    }
                    fclose(pfile);
                }
            }
        }

        if(sem_trywait(&lg_hb_sem)==0)
        {
            DEBUG(("[task_Logger] Received heartbeat request.\n"));
            /* Response to heartbeat request */
            heartbeat &= ~LG_INACTIVE;
            DEBUG(("[task_Logger] Responded to heartbeat request.\n"));
        }
    }
}
