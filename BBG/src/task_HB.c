#include "project.h"

char client_active[MAX_CLIENT_NUM][2] = {0};

static int8_t requestClientHB(uint8_t id);

void * task_HB(void * param)
{
    int i;
    char log[50];

    for( ; ; )
    {
        sleep(INTERNAL_HEARTBEAT_INTERVAL);
        sem_post(&mr_hb_sem);
        sem_post(&rx_hb_sem);
        sem_post(&tx_hb_sem);
        sem_post(&lg_hb_sem);

        for(i = 1; i<MAX_CLIENT_NUM; i++)
        {
            if(client[i].fd == -1)
                break;
            else
            {
                requestClientHB(i);
            }
        }
        sleep(INTERNAL_HEARTBEAT_INTERVAL);
        if(heartbeat & MR_INACTIVE)
        {
            serverLog(&router_q, "[ERROR] task_MsgRouter not active.");
            sem_post(&mr_sem);
        }
        if(heartbeat & RX_INACTIVE)
        {
            serverLog(&router_q, "[ERROR] task_Rx not active.");
            sem_post(&mr_sem);
        }
        if(heartbeat & TX_INACTIVE)
        {
            serverLog(&router_q, "[ERROR] task_Tx not active.");
            sem_post(&mr_sem);
        }
        if(heartbeat & LG_INACTIVE)
        {

        }
        heartbeat = 0XFF;

        for(i = 1; i<MAX_CLIENT_NUM; i++)
        {
            if(client[i].fd == -1)
                break;
            else
            {
                if(client_active[i][0]!=1)
                {
                    sprintf(log, "client[%d] noise sensor down.\n", i);
                    serverLog(&router_q, log);
                }
                if(client_active[i][1]!=1)
                {
                    sprintf(log, "client[%d] motion sensor down.\n", i);
                    serverLog(&router_q, log);
                }
            }
        }
    }
}

static int8_t requestClientHB(uint8_t id)
{
    int ret = 0;
    req_t req;
    req.id = id;
    req.src = MSG_BBB_HB;
    req.dst = MSG_TIVA_SOCKET;
    req.type = MSG_TYPE_CLIENT_HEARTBEAT_REQUEST;
    if(req_send_LINUX_mq(&router_q, &req)!=0)
        ret = -1;
    else
        sem_post(&mr_sem);
    return ret;
}
