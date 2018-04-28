#include "project.h"

char test[3] ="ab";

/* Client Socket */
int main(int argc, char const *argv[])
{
    int                socketfd;
    struct sockaddr_in serv_addr;

    /* Create a socket */
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd <0)
    {
        perror("Failed to create socket.\n");
        exit(EXIT_FAILURE);
    }

    /* Server Port attributes*/
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(SERVER_PORT);


    /* Connect to the server */
    if(connect(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==0)
  	{
        /* Send the message request */
        write(socketfd, test, strlen(test));
        sleep(5);

        /* Try reading the response 0*/
        //read(socketfd, &socket_msg_resp, sizeof(socket_msg_resp));

        if(close(socketfd)!=0)
        {
            perror("Failed to close socket connection.\n");
        }
  	}
  	else
  	{
    	perror("Failed to connect.\n");
    	exit(EXIT_FAILURE);
  	}

    exit(EXIT_SUCCESS);
}
