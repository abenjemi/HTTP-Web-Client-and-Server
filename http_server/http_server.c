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


int isDir(const char * resource) {
   struct stat buff;
   if (stat(resource, &buff) != 0)
   {
       return 0;
   }
   return S_ISDIR(buff.st_mode);
}

void send_response(char * rootdir, char * response, int sockfd, int new_fd, FILE * fptr)
{
    fseek(fptr, 0, SEEK_END);
    int size = ftell(fptr);

    fseek(fptr, 0, SEEK_SET);

    printf("200 OK\n");
    char header[1024] = {'\0'};
    sprintf(header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n\r\n", size);
    char initial_bytes[1024] = {'\0'};
    int inbytes_read = fread(initial_bytes, 1, 1024 - strlen(header), fptr);
    strcpy(response, header);
    strcat(response, initial_bytes);

    if(send(new_fd, response, 1024, MSG_NOSIGNAL) < 0) {
        // perror("send");
    }
    memset(response, '\0', 1024);
    size -= inbytes_read;
    
    while (size > 0){
        size -= fread(response, 1, 1024, fptr);
        if(send(new_fd, response, 1024, MSG_NOSIGNAL) < 0) {
            // perror("send");
        }
        memset(response, '\0', 1024);
        usleep(1000);
    }  
    
}


int main()
{
    int sockfd, new_fd;
	struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size;
    char client_ip[INET_ADDRSTRLEN];
    long bytes_read;

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
        // printf("\n+++++++ Waiting for new connection ++++++++\n\n");
		sin_size = sizeof(my_addr);

        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, (socklen_t*)&sin_size)) < 0)
        {
            perror("accept");
            continue;
        }

        // move ip address of client into client_ip
        inet_ntop(AF_INET, &(their_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        char response[1024] = {'\0'};
        char buf[1024] = {'\0'};
        bytes_read = read(new_fd , buf, 1024);
        if (buf[0] != '\0')
        {
            // char * method, resource, protocol;
            char * method = strtok(buf, " ");
            char * resource = strtok(NULL, " ");
            char * protocol = strtok(NULL, "\r\n");
            printf("%s \"%s %s %s\" ", client_ip, method, resource, protocol);

            char rootdir[1024] = {'\0'};
            strcpy(rootdir, "Webpage");
            strcat(rootdir, resource);


            if ((strstr(protocol, "HTTP/1.1") == NULL) && (strstr(protocol, "HTTP/1.0") == NULL)) // protocol is not HTTP/1.1 or HTTP/1.0
            {
                printf("501 Not Implemented - protocol\n");
                strcpy(response, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>");
                if(send(new_fd, response, 1024, 0) < 0) 
                {
                    perror("send");
                }
            }
            else if (strstr(method, "GET") == NULL) // if method used is not GET return 501
            {
                printf("501 Not Implemented\n");
                strcpy(response, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>");
                if(send(new_fd, response, 1024, 0) < 0) 
                {
                    perror("send");
                }
            }
            else if (resource[0] != '/') // if resource does not start with '/' return 400
            {
                printf("400 Bad Request - resource does not start with '/'\n");
                strcpy(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>");
                if(send(new_fd, response, 1024, 0) < 0) {
                    perror("send");
                }
            }
            else if(strstr(resource, "/../") != NULL) // if resource contains "/../" return 400
            {
                printf("400 Bad Request - resource contains /../\n");
                strcpy(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>");
                if(send(new_fd, response, 1024, 0) < 0) {
                    perror("send");
                }
            }
            else if ((strlen(resource) >= 3) && (strcmp(&resource[strlen(resource) - 3], "/..") == 0))
            {
                // char bad_string[4] = "/..";
                
                printf("400 Bad Request - resource ends with /..\n");
                strcpy(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>");
                if(send(new_fd, response, 1024, 0) < 0) {
                    perror("send");
                }
                
            }
            else if (resource[strlen(resource) - 1] == '/') // if resource ends with / append index.html to it and return this file
            {
                // printf("\n\nLast character is /\n\n");
                strcat(rootdir, "index.html");
                FILE * fptr = fopen(rootdir, "r");

                if (fptr != NULL)
                {
                    send_response(rootdir, response, sockfd, new_fd, fptr);
                    fclose(fptr);
                }
                else
                {
                    printf("404 Not Found\n");
                    strcpy(response, "HTTP/1.0 404 Not Found\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>");
                    if(send(new_fd, response, 1024, 0) < 0) {
                        perror("send");
                    }
                }
            }
            else if (isDir(rootdir))
            {
                // printf("\n It is a directory \n");
                strcat(rootdir, "/");
                strcat(rootdir, "index.html");
                // printf("\n\n %s", rootdir);

                FILE * fptr = fopen(rootdir, "r");

                if (fptr != NULL)
                {
                    send_response(rootdir, response, sockfd, new_fd, fptr);
                    fclose(fptr);
                }
                else
                {
                    printf("404 Not Found, no index\n");
                    strcpy(response, "HTTP/1.0 404 Not Found\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>");
                    if(send(new_fd, response, 1024, 0) < 0) {
                        perror("send");
                    }
                }
            }
            else
            {
                FILE * fptr = fopen(rootdir, "r");

                if (fptr != NULL)
                {
                    send_response(rootdir, response, sockfd, new_fd, fptr);
                    fclose(fptr);
                }
                else
                {
                    printf("404 Not Found\n");
                    strcpy(response, "HTTP/1.0 404 Not Found\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>");
                    if(send(new_fd, response, 1024, 0) < 0) {
                        perror("send");
                    }
                }
            }
        }
        
        close(new_fd);
    }
    return 0;
}
