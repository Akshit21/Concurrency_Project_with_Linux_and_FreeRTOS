#ifndef __TASK_H__
#define __TASK_H__

void * task_Command(void * param);
void * task_Logger(void * param);
void * task_MsgRouter(void * param);
void * task_RxSocket(void * param);
void * task_RxUART(void * param);
void * task_Tx(void * param);
void * task_HB(void * param);
#endif
