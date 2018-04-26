#ifndef __TASK_H__
#define __TASK_H__

void * task_Command(void * param);
void * task_ClientCommHandling(void * client_id);
void * task_Socket(void * param);
void * task_Serial(void * param);
void * task_ServerLogging(void * param);

#endif
