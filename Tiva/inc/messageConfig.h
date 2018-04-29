#ifndef __MESSAGECONFIG_H__
#define __MESSAGECONFIG_H__

/*
 *********** General configuration **********
 */

#define MAX_MESSAGE_LENGTH  (6)
#define MAX_LINUX_MQ_SIZE   (10)

/*
 *********** Application-specific configuration **********
 */
#define MAX_CLIENT_ID_LENGTH    (4)

/* ----- Functionality ON/OFF ----- */
#define USE_SERVER_CLIENT_MESSAGING
//#define USE_MESSAGE_OVER_LINUX_MQUEUE
#define USE_MESSAGE_OVER_FREERTOS_QUEUE
#define USE_MESSAGE_PACKET
#define USE_MESSAGE_TIMESTAMP

/* ----- Define message sources and destinations----- */
#define MSG_BBB_COMMAND                 (0x00)
#define MSG_BBB_LOGGING                 (0x01)
#define MSG_BBB_SOCKET                  (0x02)
#define MSG_BBB_MSG_EXCHANGE            (0x03)

#define MSG_TIVA_SOCKET                 (0x04)
#define MSG_TIVA_NOISE_SENSING          (0x05)
#define MSG_TIVA_MOTION_SENSING         (0x06)

#define MSG_BBB_HEARTBEAT               (0x07)

/* ----- Define message types ----- */
#define MSG_TYPE_LOG                            (0x01)
#define MSG_TYPE_SERVER_LOG                     (0x02)

#define MSG_TYPE_CLIENT_ALERT                   (0x03)
#define MSG_TYPE_CLIENT_INFO                    (0x04)
#define MSG_TYPE_CLIENT_LOG                     (0x05)

#define MSG_TYPE_SERVER_REQUEST_TO_CLIENT       (0x06)
#define MSG_TYPE_CLIENT_RESPONSE_TO_SERVER      (0x07)

#define MSG_TYPE_THREAD_HEARTBEAT_REQUEST       (0x08)
#define MSG_TYPE_THREAD_HEARTBEAT_RESPONSE      (0x09)
#define MSG_TYPE_CLIENT_HEARTBEAT_REQUEST       (0x0A)
#define MSG_TYPE_CLIENT_HEARTBEAT_RESPONSE      (0x0B)


/* ----- Defines for message packet over network ----- */
#define USER_PACKET_HEADER  (0xAA)

#endif
