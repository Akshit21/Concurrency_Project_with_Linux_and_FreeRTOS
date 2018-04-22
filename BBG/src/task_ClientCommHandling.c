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
void * task_ClientCommHandling(void * client_id)
{

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
static void clientMessageProcess(msg_t msg)
{

}
