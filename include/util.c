#include "util.h"

int connect_to_host_tcp(char *hostname,char *port){

    struct addrinfo hints;
    struct addrinfo *peer_address;

    memset(&hints,0,sizeof(hints));
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags= AI_ALL | AI_ADDRCONFIG;

    LOG_INFO("Configuring remote adress...");
    if(getaddrinfo(hostname, port, &hints, &peer_address)){
        LOG_ERR("getaddrinfo %s ",strerror(errno));
        exit(EXIT_FAILURE);   
    }
    

    LOG_INFO("Remote adress is: ");
    char adress_buf[100];
    char service_buf[100];
    getnameinfo(peer_address->ai_addr,peer_address->ai_addrlen,adress_buf,sizeof(adress_buf),service_buf,sizeof(service_buf),NI_NUMERICHOST);
    LOG_INFO("%s %s ",adress_buf,service_buf);

    LOG_INFO("Creating socket...");
    int sockfd=socket(peer_address->ai_family,peer_address->ai_socktype,peer_address->ai_protocol);
    if(sockfd==-1){
        LOG_ERR("socket  %s",strerror(errno));
        exit(EXIT_FAILURE);   
    }

    LOG_INFO("Connecting...");
    if(connect(sockfd,peer_address->ai_addr,peer_address->ai_addrlen)){
        LOG_ERR("connect  %s",strerror(errno));
        exit(EXIT_FAILURE);   
    }

    freeaddrinfo(peer_address);
    LOG_INFO("Connected...");

    return sockfd;
}


int configure_tcp_server(char *hostname,char *port){

    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_family=PF_INET;
    hints.ai_flags=AI_PASSIVE;
    struct addrinfo *bind_address;
 
    LOG_INFO("Started server at %s",port);

    LOG_INFO("Configuring local adress..");
    if(getaddrinfo(hostname,port,&hints,&bind_address))
        LOG_ERR("getaddrinfo");
   
    LOG_INFO("Creating socket..");
    int sock_listen;
    if((sock_listen=socket(bind_address->ai_family,bind_address->ai_socktype,bind_address->ai_protocol))==-1)
        LOG_ERR("socket");
    
    LOG_INFO("Binding local address...");
    if(bind(sock_listen,bind_address->ai_addr,bind_address->ai_addrlen))
        LOG_ERR("Bind");
    
    freeaddrinfo(bind_address);

    LOG_INFO("Listening...");
    if(listen(sock_listen,10))
        LOG_ERR("listen");

    return sock_listen;

}