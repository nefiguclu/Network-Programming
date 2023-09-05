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

void err(char * err_msg){
    fprintf(stderr,"%s error: %d",err_msg,errno);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]){

    if(argc<3){
        fprintf(stderr,"usage tcp_client hostname port\n");
        return -1;
    }

    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype=SOCK_STREAM;
    struct addrinfo * peer;
    if(getaddrinfo(argv[1],argv[2],&hints,&peer))
        err("getaddrinfo");

    printf("Remote adress is: ");
    char adress_buf[100];
    char service_buf[100];
    getnameinfo(peer->ai_addr,peer->ai_addrlen,adress_buf,sizeof(adress_buf),service_buf,sizeof(service_buf),NI_NUMERICHOST);
    printf("%s %s \n",adress_buf,service_buf);

    printf("Creating socket...\n");
    int socket_peer;
    if((socket_peer=socket(peer->ai_family,peer->ai_socktype,peer->ai_protocol))==-1)
        err("socket");

    printf("Connecting...\n");
    if(connect(socket_peer,peer->ai_addr,peer->ai_addrlen))
        err("connect");
    freeaddrinfo(peer);

    printf("Connected...\n");
    printf("To send data, enter text followed by enter...\n");

    char read_buf[4096];
    fd_set reads;
    fd_set copy;
    FD_ZERO(&reads);
    FD_SET(socket_peer,&reads);
    FD_SET(STDIN_FILENO,&reads);

    while(1){
         copy=reads;
        if(select(socket_peer+1,&copy,NULL,NULL,NULL)==-1)
            err("select");       

        if(FD_ISSET(STDIN_FILENO,&copy)){
            if(!fgets(read_buf,4096,stdin))
                break;
            printf("Sending %s\n",read_buf);
            int bytes_send=send(socket_peer,read_buf,strlen(read_buf),0);
            printf("%d bytes send\n",bytes_send);
        }
        if(FD_ISSET(socket_peer,&copy)){
            int bytes_recv=recv(socket_peer,read_buf,4096,0);
            if(bytes_recv<1){
                printf("connection closed by peer\n");
                break;
            }
            printf("Received %d bytes: %.*s",bytes_recv,bytes_recv,read_buf);
        }
    }

    printf("Closing socket...\n");
    close(socket_peer);

    return 0;
}