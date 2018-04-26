#include "message.h"

/**
 * @brief Send a log to the server logging task to be logged
 *
 * @param src - source of the log
 *        log - content of the log
 *
 * @return  0 - success
 *         -1 - failed.
 */
int8_t serverLog(msg_src_t src, char * log)
{

}

/**
 * @brief log a message to a specified file
 *
 * @param file_name - name of the file
 *        msg - the message structure to be logged
 *
 * @return  0 - success
 *         -1 - failed.
 */
int8_t logFile(char * file_name, msg_t msg)
{

}
