/* The code is subject to Purdue University copyright policies.
 * DO NOT SHARE, DISTRIBUTE, OR POST ONLINE
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>


int get_status(int sockfd){
    char buf[1024] = "";
    char * next_buff = buf + 1;
    int bytes;
    int status;
    printf("Begin Response ..\n");
    while(bytes = recv(sockfd, next_buff, 1, 0)){
        if(bytes < 0){
            perror("get_status");
            exit(1);
        }

        // printf("\nnext_buff[-1] = %c \n *next_buff = %c\n", next_buff[-1], *next_buff);

        if((next_buff[-1] == '\r')  && (*next_buff == '\n' )) break;
        next_buff += 1;
    }
    *next_buff = 0;
    next_buff = buf + 1;

    sscanf(next_buff, "%*s %d ", &status);

    // If it’s not ”200”, the program should print the first line of the response to the terminal (stdout) and exit.
    if (status != 200)
    {
        printf("%s\n",next_buff);
    }

    // printf("%s\n",next_buff);
    // printf("status=%d\n",status);
    // printf("End Response ..\n");
    if (bytes > 0) return status;
    return 0;
}

//the only filed that it parsed is 'Content-Length' 
int get_total_bytes(int sock){
    char buf[1024] = ""; 
    char * next_buff = buf + 4;
    int total_bytes;
    int status;
    // printf("Begin HEADER ..\n");
    while(total_bytes = recv(sock, next_buff, 1, 0)){
        if(total_bytes < 0){
            perror("Parse Header");
            exit(1);
        }

        if(
            (next_buff[-3] == '\r')  && (next_buff[-2] == '\n' ) &&
            (next_buff[-1] == '\r')  && (*next_buff == '\n' )
        ) break;
        next_buff++;
    }

    *next_buff = 0;
    next_buff = buf + 4;
    //printf("%s",ptr); not me

    if(total_bytes){
        next_buff = strstr(next_buff, "Content-Length:");
        if(next_buff)
        {
            sscanf(next_buff, "%*s %d", &total_bytes);

        }
        else
        {
            total_bytes = -1; //unknown size
            printf("Error: could not download the requested file (file length unknown)");
        }


       printf("Content-Length: %d\n", total_bytes);
    }
    // printf("End HEADER ..\n");
    return total_bytes ;

}



int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr,"usage: ./http_client [host] [port number] [filepath]\n");
        exit(1);
    }

    // create client socket
    int sockfd, numbytes;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
    
    // server information
    struct hostent* he;
	struct sockaddr_in server_addr; 
    server_addr.sin_family = AF_INET;
    short port = atoi(argv[2]);
    server_addr.sin_port = htons(port);
    // server_addr.sin_addr.s_addr = gethostbyname(argv[1]);
    
    /* get server's IP by invoking the DNS */
	if ((he = gethostbyname(argv[1])) == NULL) {
		perror("gethostbyname");
		exit(1);
	}
    server_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
    bzero(&(server_addr.sin_zero), 8);

    

    // connect with server
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0) {
		perror("connect");
		exit(1);
	}

    // printf("I got here\n");

    // printf("Sending data ...\n");
    char request[1024];

    snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\nHost: %s:%s\r\n\r\n", argv[3], argv[1], argv[2]);

    if(send(sockfd, request, strlen(request), 0) < 0){
        perror("send");
        exit(1); 
    }
    // printf("Data sent.\n");  

    //fp=fopen("received_file","wb");
    // printf("Recieving data...\n\n");

    int contentlengh;
    int bytes_received;
    char recv_data[1024];

    if(get_status(sockfd) && (contentlengh = get_total_bytes(sockfd))){

        int bytes=0;
        char * filename = strrchr(argv[3], '/');
        filename += 1;
        // printf("\n filename is %s \n", filename);
        FILE* fptr = fopen(filename,"wb");
        // printf("Saving data...\n\n");

        while(bytes_received = recv(sockfd, recv_data, 1024, 0)){
            if(bytes_received < 0){
                perror("recv");
                exit(1);
            }


            fwrite(recv_data, 1, bytes_received, fptr);
            bytes += bytes_received;
            // printf("Bytes recieved: %d from %d\n",bytes,contentlengh);
            if(bytes == contentlengh)
            break;
        }
        fclose(fptr);
    }
    else

    close(sockfd);
    printf("\n\nDone.\n\n");
    return 0;
}