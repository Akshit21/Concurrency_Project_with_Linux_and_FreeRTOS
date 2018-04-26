#ifndef __MESSAGECONFIG_H__
#define __MESSAGECONFIG_H__

/*
 *********** General configuration **********
 */

#define MAX_MESSAGE_LENGTH  (50)
#define MAX_LINUX_MQ_SIZE   (10)

/*
 *********** Application-specific configuration **********
 */

/* ----- Functionality ON/OFF ----- */
#define USE_SERVER_CLIENT_MESSAGING
#define USE_MESSAGE_OVER_LINUX_MQUEUE
// #define USE_MESSAGE_OVER_FREERTOS_QUEUE
#define USE_MESSAGE_PACKET
// #define USE_MESSAGE_TIMESTAMP

/* ----- Define message sources and destinations----- */
#define MSG_BBB_COMMAND                 (0x00)
#define MSG_BBB_LOGGING                 (0x01)
#define MSG_BBB_SOCKET                  (0x02)
#define MSG_BBB_MSG_EXCHANGE            (0x03)

#define MSG_TIVA_SOCKET                 (0x04)
#define MSG_TIVA_NOISE_SENSING          (0x05)
#define MSG_TIVA_MOTION_SENSING         (0x06)

/* ----- Define message types ----- */
#define MSG_TYPE_SERVER_LOG             (0x01)
#define MSG_TYPE_CLIENT_LOG             (0x02)
#define MSG_TYPE_SERVER_REQUEST         (0x03)
#define MSG_TYPE_CLIENT_RESPONSE        (0x04)
#define MSG_TYPE_CLIENT_ALERT           (0x05)
#define MSG_TYPE_HEARTBEAT_REQUEST      (0x06)
#define MSG_TYPE_HEARTBEAT_RESPONSE     (0x07)
#define MSG_TYPE_STATUS_UPDATE          (0x08)


/* ----- Defines for message packet over network ----- */
#define USER_PACKET_HEADER  (0xAA)

#endif
