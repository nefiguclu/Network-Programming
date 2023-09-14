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
#include <stdarg.h>
#include <ctype.h>
#include <netinet/ip_icmp.h>
#include <signal.h>


#include "../log.h"

#define MSG_SIZE 64
int loop=1;

struct packet
{
    struct icmphdr hdr;
    char message[MSG_SIZE - sizeof(struct icmphdr)];
};

struct addrinfo *dns_lookup(char *hostname);
void sigint_handler(int signum);
unsigned short checksum(void *b, int len);
char *reverse_dns_lookup(struct sockaddr *peer_adress,socklen_t len);

int main(int argc, char *argv[]){


    if(argc < 2){
        LOG_ERR("Usage: sudo ./ping hostname");
        return -1;
    }

    struct packet icmp;

    int ttl = 64;
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;


    int msgcount_send = 0;
    int msgcount_recv = 0;

    struct timespec start, end, total_start, total_end;
    struct addrinfo *remote_adress;
    remote_adress = dns_lookup(argv[1]);
    char *hostname = reverse_dns_lookup(remote_adress->ai_addr,remote_adress->ai_addrlen);

    //A user protocol may receive ICMP packets for all local sockets by opening a raw socket with the protocol IPPROTO_ICMP
    int sockfd = socket(remote_adress->ai_family, remote_adress->ai_socktype, IPPROTO_ICMP);
    if(sockfd<0){
        LOG_ERR("socket failed %s",strerror(errno));
        LOG_ERR("Need superuser privileges start with sudo");
        exit(EXIT_FAILURE);    
    }

    free(remote_adress);

    if(setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl))==-1){
        LOG_ERR("socket failed %s",strerror(errno));
        exit(EXIT_FAILURE);   
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1){
        LOG_ERR("socket failed %s",strerror(errno));
        exit(EXIT_FAILURE);   
    }

    signal(SIGINT, sigint_handler);

    clock_gettime(CLOCK_MONOTONIC_RAW, &total_start);
    while(loop){

        //construct icmp packet, will be encapsulated within the ip packet (os will handle it)
        memset(&icmp,0,sizeof(icmp));
        icmp.hdr.type=ICMP_ECHO;
        icmp.hdr.un.echo.id=getpid();
        icmp.hdr.un.echo.sequence=msgcount_send++;
        int i; 
        for(i = 0; i < sizeof(icmp.message) - 1 ; i++ ){
            icmp.message[i]= i + 'A';
        }
        icmp.message[i]= '\0';
        icmp.hdr.checksum=checksum(&icmp,sizeof(icmp));

        sleep(1); //frequency
    
        if(sendto(sockfd, &icmp, sizeof(icmp), 0, remote_adress->ai_addr,remote_adress->ai_addrlen) == -1){
            LOG_ERR("sendto %s",strerror(errno));
            continue;
        }
        else{
            clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        }

        struct sockaddr_storage sock_addr;
        socklen_t len = sizeof(sock_addr);

        if(recvfrom(sockfd,&icmp,sizeof(icmp),0,(struct sockaddr*)&sock_addr,&len) == -1){
            LOG_ERR("recvfrom %s",strerror(errno));
        }
        else{
            //if(icmp.hdr.type == 69 && icmp.hdr.code == 0) { A bug was observed regarding type and code during the tests
                clock_gettime(CLOCK_MONOTONIC_RAW, &end);
                msgcount_recv++;
                long double deltaus = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
                LOG_INFO("%d bytes from %s icmp_seq=%d ttl=%d time=%.2Lf ms",MSG_SIZE,hostname,msgcount_send,ttl,deltaus);
            //}
        }
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &total_end);
    long double deltaus_total = (total_end.tv_sec - total_start.tv_sec) * 1000.0 + (total_end.tv_nsec - total_start.tv_nsec) / 1000000.0;
    LOG_INFO("\n--- %s ping statistics ---\n",hostname);
    LOG_INFO("%d packets transmitted, %d received, %s%d packet loss, time %.2Lf ms",msgcount_send,msgcount_recv, "%",
    (100 * (msgcount_send - msgcount_recv)) / (msgcount_recv + msgcount_send) ,deltaus_total);

    close(sockfd);
    return 0;
}


struct addrinfo *dns_lookup(char *hostname) {

    struct addrinfo hints;
    struct addrinfo *peer_adress;
    
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype=SOCK_RAW;
    hints.ai_family= AF_INET;
    hints.ai_flags= AI_ADDRCONFIG;


    if(getaddrinfo(hostname,NULL,&hints,&peer_adress)){
        LOG_ERR("getaddrinfo failed %s",strerror(errno));
        exit(EXIT_FAILURE);
    }
    return peer_adress;
}

char *reverse_dns_lookup(struct sockaddr *peer_adress,socklen_t len) {
    static char host[254];
    if(getnameinfo(peer_adress,len,host,sizeof(host),NULL,0,0)){
        LOG_ERR("getaddrinfo failed %s",strerror(errno));
    }
    return host;
}

unsigned short checksum(void *b, int len)
{    
    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;

 // Sum up 2-byte values until none or only one byte left.
    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    result = ~sum;
    return result;
}


void sigint_handler(int signum){
    loop = 0;
}