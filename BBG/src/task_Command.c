#include "project.h"

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

}
