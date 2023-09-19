
#include "util.h"


void err(char * err_msg){
    LOG_ERR("%s: %d \n",err_msg,strerror(errno));
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]){

    int sock_listen = configure_tcp_server(NULL,"8080");
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

    LOG_INFO("Waiting for a connection...");

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
                    LOG_INFO("Client connected: %s %s",host_addr,serv_addr);

                    FD_SET(peer_sock,&master);
                    peer_sock>max?max=peer_sock:max; 
                }
                else{
                    bytes_received=recv(i,read_buf,1024,0);
                    if(bytes_received==-1){
                        LOG_INFO("connection closed by client closing socket...");
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