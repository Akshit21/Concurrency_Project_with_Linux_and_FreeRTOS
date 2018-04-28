#include "project.h"

/**
 * @brief Client communication handling task
 *
 * This task is per connected client. It dedicates a mqueue for each connected
 * client. The specific client communication handling is based on a client_id
 * supplied when creating the task. When the client disconnects, this task
 * should unlink the mqueue and then exit.
 *
 * @param client_id - unique client id for different communication handling.
 *
 * @return none.
 */
void * task_MsgRouter(void * param)
{
    msg_t           temp_msg;
    struct timespec wait_time =
                    {
                        .tv_sec = 1;
                        .tv_nsec = 0;
                    }

    for( ; ; )
    {
        if(sem_timedwait(&mr_sem, &wait_time)==0)
        {
            /* Messages pending to be routed */
            if(msg_receive_LINUX_mq(&router_q, &temp_msg)!=0)
            {
                perror("[ERROR] [task_MsgRouter] mq_receive() failed.\n");
            }

            /* Route the processed message accordingly */
            if(processMessage(&temp_msg)!=0)
            {
                perror("[ERROR] [task_MsgRouter] Failed to route client message.\n");
            }
            else
                DEBUG(("[task_MsgRouter] Routed client message.\n"));
        }

        if(sem_trywait(&mr_hb_sem)==0)
        {
            DEBUG(("[task_MsgRouter] Received heartbeat request.\n"));
            /* Response to heartbeat request */

            DEBUG(("[task_MsgRouter] Responded to heartbeat request.\n"));
        }

    }
}

/**
 * @brief message process for client0
 *
 * @param msg - message from the client.
 *
 * - Accept request commands from the command task and route the requests to
 *   the socket task to be sent out to the corresponding client
 * - Accept client responses via the socket task and route the reponses to the
 *   command task for displayed
 * - Accept logs from clients and route them to the logging task to be logged
 * - Accept heartbeat requests from main and route them to the corresponding client
 * - Accept heartbeat responses from a client and route them back to main
 */
static int8_t processMessage(msg_t * msg)
{
    switch (msg->type)
    {
        case MSG_TYPE_LOG:
            /* Route message to logger */
            if(msg_send_LINUX_mq(&logger_q, msg)!=0)
            {
                perror("[ERROR] [task_MsgRouter] Failed to route logs.\n");
            }
            /* Notify the logger task */
            sem_post(&lg_sem);
            break;
        case MSG_TYPE_SERVER_REQUEST_TO_CLIENT:
            /* Populate the txbuf and notify the task_Tx to send out the request */
            txbuf = *msg;
            sem_post(&tx_sem);
            break;
        case MSG_TYPE_CLIENT_RESPONSE_TO_SERVER:
            if(msg->src == MSG_TIVA_NOISE_SENSING)
                response[0] = *msg;
            else if(msg->src == MSG_TIVA_MOTION_SENSING)
                response[1] = *msg;
            sem_post(&cm_sem);
            break;
        case MSG_TYPE_THREAD_HEARTBEAT_REQUEST:
            break;
        case MSG_TYPE_THREAD_HEARTBEAT_RESPONSE:
            break;
        case MSG_TYPE_CLIENT_HEARTBEAT_REQUEST:
            break;
        case MSG_TYPE_CLIENT_HEARTBEAT_RESPONSE:
            break;
        default:;
    }
}
