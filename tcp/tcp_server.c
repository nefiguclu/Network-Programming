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
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_family=PF_INET;
    hints.ai_flags=AI_PASSIVE;
    struct addrinfo *bind_address;
 
    printf("\n\nStarted server at 8080\n\n");

    printf("Configuring local adress..\n");
    if(getaddrinfo(NULL,"8080",&hints,&bind_address))
        err("getaddrinfo");
   
    printf("Creating socket..\n");
    int sock_listen;
    if((sock_listen=socket(bind_address->ai_family,bind_address->ai_socktype,bind_address->ai_protocol))==-1)
        err("socket");
    
    printf("Binding local address..\n");
    if(bind(sock_listen,bind_address->ai_addr,bind_address->ai_addrlen))
        err("Bind");
    
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if(listen(sock_listen,10))
        err("listen");

    fd_set master;
    fd_set read;

    int peer_sock;
    struct sockaddr_storage peer_addr;
    socklen_t peer_len=sizeof(peer_addr);

    char host_addr[100];
    char serv_addr[100];

    char read_buf[1024];
    int bytes_received;

    int max=sock_listen;
    FD_ZERO(&master);
    FD_SET(sock_listen,&master);

    printf("Waiting for a connection...\n");
    while(1){

        read=master;
        if(select(max+1,&read,NULL,NULL,NULL)==-1)
            err("select");
        for(int i=1; i<max+1; i++){
            if(FD_ISSET(i,&read)){
                if(i==sock_listen){
                    if((peer_sock=accept(sock_listen,(struct sockaddr*)&peer_addr,&peer_len))==-1)
                        err("accept");
                    if(getnameinfo((struct sockaddr*)&peer_addr,peer_len,host_addr,sizeof(host_addr),serv_addr,sizeof(serv_addr),NI_NUMERICHOST))
                        err("getnameinfo");
                    printf("Client connected: %s %s\n",host_addr,serv_addr);

                    FD_SET(peer_sock,&master);
                    peer_sock>max?max=peer_sock:max; 
                }
                else{
                    bytes_received=recv(i,read_buf,1024,0);
                    if(bytes_received==-1){
                        printf("connection closed by client closing socket...\n");
                        FD_CLR(i,&master);
                        close(i);
                        continue;
                    }
                    for(int j=1;j<max+1;++j){
                        if(FD_ISSET(j,&master)){
                            if(j==sock_listen || j==i)
                                continue;
                            else
                                send(j,read_buf,bytes_received,0);
                        }
                    }                     
                }
            }   
        }
    }

    close(sock_listen);
    return 0;

}