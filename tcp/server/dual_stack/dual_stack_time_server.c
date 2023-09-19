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


int main(){

    printf("Configuring local adress...\n");
    struct addrinfo hints;
    memset((void *) &hints,0,sizeof(hints));
    hints.ai_family=PF_INET6;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_PASSIVE;

    printf("\n\nStarted server at 8080\n\n");
    struct addrinfo *bind_address;
    getaddrinfo(NULL,"8080",&hints,&bind_address);

    printf("Creating socket...\n");
    int listen_socket=socket(bind_address->ai_family,bind_address->ai_socktype,bind_address->ai_protocol);
    if(listen_socket==-1){
        fprintf(stderr,"socket failed with error %d\n",errno);
        return 1;
    }

    //dual stack 
    //clear IPV6_V6ONLY opt
    int option=0;
    if(setsockopt(listen_socket,IPPROTO_IPV6,IPV6_V6ONLY,(void *)&option,sizeof(option))){
        fprintf(stderr,"setsockopt failed with error %d\n",errno);
        return 1;      
    }

    printf("Binding socket to local adress... \n");
    if(bind(listen_socket,bind_address->ai_addr,bind_address->ai_addrlen)){
        fprintf(stderr,"bind failed with error %d\n",errno);
        return 1;
    }

    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if(listen(listen_socket,10)){
        fprintf(stderr,"listen failed with error %d\n",errno);
        return 1;           
    }

    printf("Waiting for a connection...\n");

    struct sockaddr_storage client_address;
    socklen_t address_len=sizeof(client_address);
    int client_socket;
    client_socket=accept(listen_socket,(struct sockaddr *)&client_address,&address_len);

    if(client_socket==-1){
        fprintf(stderr,"socket failed with error %d\n",errno);
        return 1;
    }

    printf("Client is connected...");
    char buffer[NI_MAXHOST];
    if(getnameinfo((struct sockaddr *)&client_address,address_len,buffer,NI_MAXHOST,0,0,NI_NUMERICHOST)){
        fprintf(stderr,"getnameinfo failed with error %d\n",errno);
        return 1;        
    }
    printf("%s\n",buffer);

    printf("Reading request...\n");
    char request[1024];
    int bytes_received=recv(client_socket,request,1024,0);
    printf("Received %d bytes....\n ",bytes_received);
    printf("%.*s",bytes_received,request);

    printf("Sending response...\n");

    const char *response=
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Content-Type: text/plain\r\n\r\n"
    "Local time is: ";

    int bytes_send=send(client_socket,response,strlen(response),0);

    time_t timer;
    time(&timer);
    char *time_msg=ctime(&timer);
    send(client_socket,time_msg,strlen(time_msg),0);

    printf("Closing connection...\n");
    close(client_socket);

    printf("Closing listening socket...\n");
    close(listen_socket);

    printf("finished\n");
    return 0;
}