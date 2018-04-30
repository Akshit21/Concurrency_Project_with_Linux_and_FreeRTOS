#include "project.h"

static int8_t processMessage(msg_t * msg);

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
                        .tv_sec = 1,
                        .tv_nsec = 0,
                    };

    for( ; ; )
    {
        if(sem_timedwait(&mr_sem, &wait_time)==0)
        {
            /* Messages pending to be routed */
            if(msg_receive_LINUX_mq(&router_q, &temp_msg)!=0)
            {
                errorHandling(0, "[ERROR] [task_MsgRouter] Failed to dequeue.");
                // perror("[ERROR] [task_MsgRouter] mq_receive() failed.\n");
            }

            /* Route the processed message accordingly */
            if(processMessage(&temp_msg)!=0)
            {
                errorHandling(0, "[ERROR] [task_MsgRouter] Failed to route client message.");
                //perror("[ERROR] [task_MsgRouter] Failed to route client message.\n");
            }
            else
                DEBUG(("[task_MsgRouter] Routed client message.\n"));
        }

        if(sem_trywait(&mr_hb_sem)==0)
        {
            DEBUG(("[task_MsgRouter] Received heartbeat request.\n"));
            /* Response to heartbeat request */
            heartbeat &= ~MR_INACTIVE;
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
    int8_t ret = 0;
    switch (msg->type)
    {
        case MSG_TYPE_LOG:
            /* Route message to logger */
            DEBUG(("[task_MsgRouter] Routing log message.\n"));
            if(msg_send_LINUX_mq(&logger_q, msg)!=0)
            {
                perror("[ERROR] [task_MsgRouter] Failed to route logs.\n");
                ret = -1;
            }
            /* Notify the logger task */
            sem_post(&lg_sem);
            break;
        case MSG_TYPE_SERVER_REQUEST_TO_CLIENT:
        case MSG_TYPE_CLIENT_HEARTBEAT_REQUEST:
            /* Populate the txbuf and notify the task_Tx to send out the request */
            DEBUG(("[task_MsgRouter] Routing out server request.\n"));
            txbuf = *(req_t*)msg;
            sem_post(&tx_sem);
            break;
        case MSG_TYPE_CLIENT_RESPONSE_TO_SERVER:
	        DEBUG(("[task_MsgRouter] Routing client response.\n"));
            if(msg->src == MSG_TIVA_NOISE_SENSING)
                response[0] = *msg;
            else if(msg->src == MSG_TIVA_MOTION_SENSING)
                response[1] = *msg;
            sem_post(&cm_sem);
            break;
        case MSG_TYPE_CLIENT_HEARTBEAT_RESPONSE:
            DEBUG(("[task_MsgRouter] Received HB response from client.\n"));
            client_active[msg->id][0] = msg->content[0];
            client_active[msg->id][1] = msg->content[1];
            break;
        default:;
    }
    return ret;
}
