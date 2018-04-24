#ifndef __UTIL_H__
#define __UTIL_H__

int8_t serverLog(msg_src_t src, char * log);

int8_t logFile(char * file_ame, msg_t msg);

int8_t validateCRC(msg_t msg, crc_t crc);

#endif
