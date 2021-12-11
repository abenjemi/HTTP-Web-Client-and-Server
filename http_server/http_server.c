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


int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}


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
        // printf("\n+++++++ Waiting for new connection ++++++++\n\n");
		sin_size = sizeof(my_addr);

        if ((new_fd = accept(sockfd, (struct sockaddr *)&my_addr, (socklen_t*)&sin_size))<0)
        {
            perror("accept");
            exit(1);
        }

        // printf("\nIM HERE\n");

        char response[1000];
        char buf[30000] = {'\0'};
        bytes_read = read(new_fd , buf, 30000);
        if (buf[0] != '\0')
        {
            // char * method, resource, protocol;
            char * method = strtok(buf, " ");
            char * resource = strtok(NULL, " ");
            char * protocol = strtok(NULL, "\r\n");
            fprintf(stdout, "\"%s %s %s\" ", method, resource, protocol);

            char rootdir[1024] = {0};
            strcpy(rootdir, "Webpage");
            strcat(rootdir, resource);

            if (strstr(method, "GET") == NULL) // if method used is not GET return 501
            {
                printf("501 Not Implemented\n");
                strcpy(response, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>");
                if(send(new_fd, response, 1000, 0) < 0) 
                {
                    perror("send");
                }
            }
            else if (resource[0] != '/') // if resource does not start with '/' return 400
            {
                printf("400 Bad Request - resource does not start with '/'\n");
                strcpy(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>");
                if(send(new_fd, response, 1000, 0) == -1) {
                    perror("send");
                }
            }
            else if(strstr(resource, "/../") != NULL) // if resource contains "/../" return 400
            {
                printf("400 Bad Request - resource contains /../\n");
                strcpy(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>");
                if(send(new_fd, response, 1000, 0) == -1) {
                    perror("send");
                }
            }
            else if (strlen(resource) >= 3)
            {
                char bad_string[4] = "/..";
                if (strcmp(&resource[strlen(resource) - 3], bad_string) == 0) // if resource ends with "/.." return 400
                {
                    printf("400 Bad Request - resource ends with /..\n");
                    strcpy(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>");
                    if(send(new_fd, response, 1000, 0) == -1) {
                        perror("send");
                    }
                }
            }
            else if (resource[strlen(resource) - 1] == '/')
            {
                // printf("\n\n FIRST CHARACTER OF PATH IS    %c\n\n", resource[0]);
                strcat(rootdir, "index.html");

                FILE * fptr = fopen(rootdir, "r");

                if (fptr != NULL)
                {
                    fseek(fptr, 0, SEEK_END);
                    int size = ftell(fptr);

                    fseek(fptr, 0, SEEK_SET);

                    printf("200 OK\n");
                    char header[1000];
                    memset(header, '\0', 1000);
                    sprintf(header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n\r\n", size);
                    char tmp[1000];
                    memset(tmp, '\0', 1000);
                    int lenread = fread(tmp, 1, 1000 - strlen(header), fptr);
                    strcpy(response, header);
                    for(int i = 0; i < lenread; i++){
                        response[i + strlen(header)] = tmp[i];
                    }

                    if(send(new_fd, response, 1000, 0) < 0) {
                        perror("send");
                    }
                    memset(response, '\0', 1000);
                    size -= lenread;
                    
                    while (size > 0){
                        size -= fread(response, 1, 1000, fptr);
                        if(send(new_fd, response, 1000, 0) == -1) {
                            perror("send");
                        }
                        memset(response, '\0', 100);
                        usleep(1000);
                    }
                    
                    fclose(fptr);
                }
                else
                {
                    fprintf(stdout, " 404 Not Found\n");
                    strcpy(response, "HTTP/1.0 404 Not Found\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>");
                    if(send(new_fd, response, 1000, 0) == -1) {
                        perror("send");
                    }
                    memset(response, '\0', 1000);
                }

            }
            // else if ()

            // printf("%s\n",buf);
            // ssize_t bytes = write(new_fd , hello , strlen(hello));
            // printf("------------------Hello message sent-------------------");

        }
        
        close(new_fd);
    }
    return 0;
}
