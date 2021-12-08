/* The code is subject to Purdue University copyright policies.
 * DO NOT SHARE, DISTRIBUTE, OR POST ONLINE
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MYPORT 8080 /* server should bind to port 8080 */
                    /* bind to IP address INADDR_ANY */

#define MAXPENDING 50 /* Maximum outstanding connection requests; listen() param */

#define DBADDR "127.0.0.1"
#define DBPORT 53004


int main()
{
    int sockfd, new_fd;
	struct sockaddr_in my_addr;
    int sin_size;
    char dst[INET_ADDRSTRLEN];
    long bytes_read;

    char *hello = "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello world!";

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

    
    my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYPORT);
	my_addr.sin_addr.s_addr = INADDR_ANY; /* bind to all local interfaces */
	bzero(&(my_addr.sin_zero), 8);

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, MAXPENDING) < 0) {
		perror("listen");
		exit(1);
	}


    while(1) {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");
		sin_size = sizeof(my_addr);

        if ((new_fd = accept(sockfd, (struct sockaddr *)&my_addr, (socklen_t*)&sin_size))<0)
        {
            perror("accept");
            exit(1);
        }

        printf("\nIM HERE\n");

        char buf[30000] = {0};
        bytes_read = read(new_fd , buf, 30000);
        printf("%s\n",buf);
        ssize_t bytes = write(new_fd , hello , strlen(hello));
        printf("------------------Hello message sent-------------------");
        close(new_fd);
    }
    return 0;
}
