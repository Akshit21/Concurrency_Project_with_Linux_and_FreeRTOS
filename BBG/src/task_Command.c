#include "project.h"

#ifdef SOCKET
#define INITIAL	(1)
#else
#define INITIAL (0)
#endif

msg_t response[2];

static int8_t requestNoiseLevel(uint8_t client_id);
static int8_t requestMotionDetection(uint8_t client_id);

/**
 * @brief User commands and interaction handling task
 *
 * This task provides a menu for user interaction. Upon new client connection,
 * the menu should pops up corresponding menu options for user to interract with
 * the client like requesting information. When the connection is off, the
 * corresponding menu options also disappear. New command can be issued only
 * after previous command has been responded or timedout.
 *
 * @param none.
 *
 * @return none.
 */
void * task_Command(void * param)
{
    int32_t         i;
    uint8_t         request_client_id, request_type;
    struct timespec wait_time;

    for( ; ; )
    {
        /* Print the menu
           Only display options for connected clients.
        */
        printf("****** USER MENU ******\n");
        for(i=INITIAL; i<MAX_CLIENT_NUM; i++)
        {
            if(client[i].fd == -1)
                break;
            printf("client[%d]: 0 - Noise 1 - Motion\n", i);
        }
        if(i == 1)
        {
            printf("There is currently no client connected...\n");
            printf("Press 'Enter' to refresh the menu.\n");
            getchar();
            continue;
        }
        else
        {
            /* Wait for user input */
            printf("\nPlease specify the client number to request: ");
            scanf("%u", &request_client_id);
	    printf("inpit:%d\n", request_client_id);
            printf("\nPlease specify the request type: ");
            scanf("%u", &request_type);

            if(client[request_client_id].fd != -1)
            {
                /* Enqueue the request */
                switch (request_type)
                {
                    case 0:
                        requestNoiseLevel(request_client_id);
                        /* Setup timeout duration */
                        if (clock_gettime(CLOCK_REALTIME, &wait_time) == -1)
                        {
                            perror("clock_gettime");
                        }
                        wait_time.tv_sec += 5;
                        /* Wait for client respnse for the timeout duration */
                        if(sem_timedwait(&cm_sem, &wait_time)==0)
                        {
                            printf("Response: client[%d] noise level: %s.\n",
                                    request_client_id, response[request_type].content);
                        }
                        else
                            printf("Timeout. PLease try again.\n");
                        break;
                    case 1:
                        requestMotionDetection(request_client_id);
                        /* Setup timeout duration */
                        if (clock_gettime(CLOCK_REALTIME, &wait_time) == -1)
                        {
                            perror("clock_gettime");
                        }
                        wait_time.tv_sec += 5;
                        /* Wait for client respnse for the timeout duration */
                        if(sem_timedwait(&cm_sem, &wait_time)==0)
                        {
                            printf("Response: client[%d] motion status: %s.\n",
                                    request_client_id, response[request_type].content);
                        }
                        else
                            printf("Timeout. Please try again.\n");
                        break;
                    default:
                        printf("Invalid request type.\n");
                }
            }
            else
            {
                printf("Requested client not active.\n");
            }
        }
    }
}

/**
 * @brief Request noise level from client
 *
 * The function send out a request for the current noise level to a speified
 * client via the corresponding client communication handling task
 *
 * @param client_id - the id of the client to request the noise level from.
 *
 * @return  0 - success
 *         -1 - failed.
 */
static int8_t requestNoiseLevel(uint8_t client_id)
{
    int8_t ret = 0;
    msg_t msg;
    msg.id = client_id;
    msg.src = MSG_BBB_COMMAND;
    msg.dst = MSG_TIVA_NOISE_SENSING;
    msg.type = MSG_TYPE_SERVER_REQUEST_TO_CLIENT;
    getTimestamp(msg.timestamp);
    msg.content[0] = '1';
    if(msg_send_LINUX_mq(&router_q, &msg)!=0)
        ret = -1;
    else
        sem_post(&mr_sem);
    return ret;
}

/**
 * @brief Request noise level from client
 *
 * The function send out a request for the current noise level to a speified
 * client via the corresponding client communication handling task
 *
 * @param client_id - the id of the client to request the noise level from.
 *
 * @return  0 - success
 *         -1 - failed.
 */
static int8_t requestMotionDetection(uint8_t client_id)
{
    int8_t ret = 0;
    msg_t msg;
    msg.id = client_id;
    msg.src = MSG_BBB_COMMAND;
    msg.dst = MSG_TIVA_MOTION_SENSING;
    msg.type = MSG_TYPE_SERVER_REQUEST_TO_CLIENT;
    getTimestamp(msg.timestamp);
    msg.content[0] = '2';
    if(msg_send_LINUX_mq(&router_q, &msg)!=0)
        ret = -1;
    else
        sem_post(&mr_sem);
    return ret;
}
