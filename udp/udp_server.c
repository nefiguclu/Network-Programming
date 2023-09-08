#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/select.h>
#include <ctype.h>

void err(char * err_msg){
    fprintf(stderr,"%s error: %d \n",err_msg,errno);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]){


    struct addrinfo hints;
    hints.ai_family=PF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE;
    struct addrinfo *bind_address;

    printf("\n\nStarted server at 8080\n\n");


    if(getaddrinfo(NULL,"8080",&hints,&bind_address))
        err("getaddrinfo");

    int sock_listen;
    if((sock_listen=socket(bind_address->ai_family,bind_address->ai_socktype,bind_address->ai_protocol))==-1)
        err("socket");
    
    if(bind(sock_listen,bind_address->ai_addr,bind_address->ai_addrlen)==-1)
        err("bind");

    freeaddrinfo(bind_address);


    struct sockaddr_storage client_address;
    socklen_t client_address_len=sizeof(client_address);
    char read_buf[1024];
    int bytes_received;

    fd_set master;
    fd_set reads;
    int max;
    max=sock_listen;
    FD_ZERO(&master);
    FD_SET(sock_listen,&master);

    while(1){
        reads=master;
        if(select(max+1,&reads,NULL,NULL,NULL)==-1)
            err("select");

       if(FD_ISSET(sock_listen,&reads)){
            printf("Client connected....\n");
            bytes_received=recvfrom(sock_listen,read_buf,sizeof(read_buf),0,(struct sockaddr*)&client_address,&client_address_len);
            if(bytes_received<1){
                printf("connection closed by peer\n");
                continue;
            }

            printf("Received %d bytes: %.*s",bytes_received,bytes_received,read_buf);

            for(int j=0;j<bytes_received;j++)
                read_buf[j]=toupper(read_buf[j]);
            int bytes_send=sendto(sock_listen,read_buf,bytes_received,0,(struct sockaddr*)&client_address,client_address_len);
            printf("Send %d bytes...\n",bytes_send);

       }
    }


    close(sock_listen);
    return 0;
}