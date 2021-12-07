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


int ReadHttpStatus(int sockfd){
    char c;
    char buff[1024]="",*ptr=buff+1;
    int bytes_received, status;
    printf("Begin Response ..\n");
    while(bytes_received = recv(sockfd, ptr, 1, 0)){
        if(bytes_received==-1){
            perror("ReadHttpStatus");
            exit(1);
        }

        if((ptr[-1]=='\r')  && (*ptr=='\n' )) break;
        ptr++;
    }
    *ptr=0;
    ptr=buff+1;

    sscanf(ptr,"%*s %d ", &status);

    printf("%s\n",ptr);
    printf("status=%d\n",status);
    printf("End Response ..\n");
    return (bytes_received>0)?status:0;

}

//the only filed that it parsed is 'Content-Length' 
int ParseHeader(int sock){
    char c;
    char buff[1024]="",*ptr=buff+4;
    int bytes_received, status;
    printf("Begin HEADER ..\n");
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received==-1){
            perror("Parse Header");
            exit(1);
        }

        if(
            (ptr[-3]=='\r')  && (ptr[-2]=='\n' ) &&
            (ptr[-1]=='\r')  && (*ptr=='\n' )
        ) break;
        ptr++;
    }

    *ptr=0;
    ptr=buff+4;
    //printf("%s",ptr);

    if(bytes_received){
        ptr=strstr(ptr,"Content-Length:");
        if(ptr){
            sscanf(ptr,"%*s %d",&bytes_received);

        }else
            bytes_received=-1; //unknown size

       printf("Content-Length: %d\n",bytes_received);
    }
    printf("End HEADER ..\n");
    return  bytes_received ;

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
		printf("socket");
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
		printf("gethostbyname");
		exit(1);
	}
    server_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
    bzero(&(server_addr.sin_zero), 8);

    

    // connect with server
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0) {
		perror("connect");
		exit(1);
	}

    printf("I got here\n");

    printf("Sending data ...\n");
    char request[1024];

    snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\nHost: %s:%s\r\n\r\n", argv[3], argv[1], argv[2]);

    if(send(sockfd, request, strlen(request), 0) < 0){
        perror("send");
        exit(1); 
    }
    printf("Data sent.\n");  

    //fp=fopen("received_file","wb");
    printf("Recieving data...\n\n");

    int contentlengh;
    int bytes_received;
    char recv_data[1024];

    if(ReadHttpStatus(sockfd) && (contentlengh=ParseHeader(sockfd))){

        int bytes=0;
        FILE* fptr = fopen("make.html","wb");
        printf("Saving data...\n\n");

        while(bytes_received = recv(sockfd, recv_data, 1024, 0)){
            if(bytes_received==-1){
                perror("recieve");
                exit(3);
            }


            fwrite(recv_data,1,bytes_received,fptr);
            bytes+=bytes_received;
            printf("Bytes recieved: %d from %d\n",bytes,contentlengh);
            if(bytes==contentlengh)
            break;
        }
        fclose(fptr);
    }
    close(sockfd);
    printf("\n\nDone.\n\n");
    return 0;
}