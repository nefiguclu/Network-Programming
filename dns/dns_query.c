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

#include "../log.h"


void printDnsMessage(const unsigned char *message,int length);
const unsigned char *print_name(const char *msg,const unsigned char *p,const char *end);

int main(int argc, char *argv[]){

    if (argc < 3) {
        LOG_ERR("Usage: dns_query hostname type");
        exit(EXIT_FAILURE);
    }

    if (strlen(argv[1]) > 255) {
        LOG_ERR("Usage: dns_query hostname type");
        exit(EXIT_FAILURE);
    }

    unsigned char type;
    if (strcmp(argv[2], "a") == 0) {
        type = 1;
    } else if (strcmp(argv[2], "aaaa") == 0) {
        type = 28;
    } else {
        LOG_ERR("Unknown type '%s'. Use a, aaaa",argv[2]);
        exit(EXIT_FAILURE);
    }

    LOG_INFO("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo *peer_address;
    if (getaddrinfo("8.8.8.8", "53", &hints, &peer_address)) {
        LOG_ERR("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }


    LOG_INFO("Creating socket");
    int socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (socket_peer < 0) {
        LOG_ERR("socket() failed");
        exit(EXIT_FAILURE);
    }


//constructing header
    char query[1024] = {0xAB, 0xCD, /* ID */
                        0x01, 0x00, /* Set recursion */
                        0x00, 0x01, /* QDCOUNT */
                        0x00, 0x00, /* ANCOUNT */
                        0x00, 0x00, /* NSCOUNT */
                        0x00, 0x00 /* ARCOUNT */};

//constructing question
    char *p = query + 12;
    char *h = argv[1];

    while(*h) {
        char *len = p;
        p++;

        if (h != argv[1]) 
            ++h;

        while(*h && *h != '.') 
            *p++ = *h++;

        *len = p - len - 1;
    }

    *p++ = '\0';

    *p++ = 0x00; 
    *p++ = type; //QTYPE

    *p++ = 0x00; 
    *p++ = 0x01; //QCLASS


    const int query_size = p - query;

    int bytes = sendto(socket_peer,query, query_size,0,peer_address->ai_addr, peer_address->ai_addrlen);
    LOG_INFO("Sent %d bytes.\n", bytes);
    printDnsMessage(query, query_size);

    char read[1024];
    int bytes_received = recvfrom(socket_peer, read, 1024, 0, 0, 0);

    LOG_INFO("Received %d bytes.\n", bytes_received);

    printDnsMessage(read, bytes_received);
    printf("\n");


    freeaddrinfo(peer_address);
    close(socket_peer);

    return 0;
}


void printDnsMessage(const unsigned char *message,int length){

    if(length<12){
        LOG_ERR("Not a valid DNS message, it should at least 12 bytes long");
        exit(EXIT_FAILURE);
    }

    const unsigned char *p = message;

    LOG_DBG("Raw DNS message");
    for(int i=0 ; i<length ; i++){
        printf("%02X",p[i]);
    }
    printf("\n");


//parsing header
    LOG_INFO("ID = %0X %0X",p[0],p[1]);

    int qr = (p[2] & 0x80) >> 7;
    int opcode = (p[2] & 0x78 ) >> 3;
    int aa = (p[2] & 0x04 ) >> 2;
    int tc = (p[2] & 0x02 ) >> 1;
    int rd = (p[2] & 0x01 );

    int ra = (p[3] & 0x80 ) >> 7;
    int rcode = (p[3] & 0x0F );

    LOG_INFO("qr = %d",qr);
    LOG_INFO("opcode = %d",opcode);
    LOG_INFO("aa = %d",aa);
    LOG_INFO("tc = %d",tc);
    LOG_INFO("rd = %d",rd);
    LOG_INFO("ra = %d",ra);

    if(qr){
        LOG_INFO("rcode = %d",rcode);
        switch(rcode){
            case 0:
                LOG_INFO("No error");
                break;
            case 1:
                LOG_INFO("Format error");
                break;
            case 2:
                LOG_INFO("Server error");
                break;
            case 3:
                LOG_INFO("Name error");
                break;
            case 4:
                LOG_INFO("Not implemented");
                break;
            case 5:
                LOG_INFO("Refused");
                break;
            default:
                LOG_INFO("?");
                break;
        }
        if(rcode)
            return;
    }

    int qdcount = (p[4] << 8) + p[5];
    int ancount = (p[6] << 8) + p[7];
    int nscount = (p[8] << 8) + p[9];
    int arcount = (p[10] << 8) + p[11];

    LOG_INFO("qdcount = %d",qdcount);
    LOG_INFO("ancount = %d",ancount);
    LOG_INFO("nscount = %d",nscount);
    LOG_INFO("arcount = %d",arcount);

    p+=12;
    const char *end=message + length;
//parsing question
    if(qdcount){
        LOG_INFO("**********Question**********");
        LOG_INFO("Name");
        p = print_name(message,p,end);

        int qtype = ( p[0] << 8 ) + p[1];
        LOG_INFO("qtype %d",qtype);
        int qclass = ( p[2] << 8 ) + p[3];
        LOG_INFO("qclass %d",qclass);
        p+=4;
    }


//parsing answer
    if( ancount || nscount || arcount ) {
    
        int i;
        for( i=0 ; i < ancount + nscount + arcount ; i++){

            if(p  >= end){
                LOG_ERR("end of message");
                exit(EXIT_FAILURE);
            }

            LOG_INFO("**********ANS %d**********",i+1);
            LOG_INFO("***Name");
            p = print_name(message,p,end);

            if(p + 10 > end){
                LOG_ERR("end of message");
                exit(EXIT_FAILURE);
            }

            int type = ( p[0] << 8 ) + p[1];
            LOG_INFO("type %d",type);

            int class = ( p[2] << 8 ) + p[3];
            LOG_INFO("class %d",class);

            int ttl = ( p[4] << 24 ) + (p[5] << 16) +
                        (p[6] << 8) + (p[7]);
            LOG_INFO("ttl %d",ttl);


            int rdlen = ( p[8] << 8 ) + p[9];
            LOG_INFO("rdlen %d",rdlen);
            p+=10;

            if(p + rdlen > end){
                LOG_ERR("end of message");
                exit(EXIT_FAILURE);
            }
            if(rdlen == 4 && type == 1 ){ //A record
                LOG_INFO("Adress");
                LOG_INFO("%d.%d.%d.%d",p[0],p[1],p[2],p[3]);


            }
            else if(rdlen == 16 && type == 28 ){ //AAAA record
                LOG_INFO("Adress");
                for(int j=0; j< rdlen ; j+=2){
                    printf("%02x%02x",p[j],p[j+1]);
                    if(j + 2 < rdlen) 
                        printf(":");
                }
                printf("\n");
            }

        }
    }

    if( p != end) 
        LOG_INFO("There is some unread data");

    printf("\n");


}

const unsigned char *print_name(const char *msg,const unsigned char *p,const char *end){

    if(p + 2  > end){
        LOG_ERR("end of message");
        exit(EXIT_FAILURE);
    }

    if((p[0] & 0xc0) == 0xc0){
        int offset= ((p[0] & 0x3F) << 8) + p[1];
        print_name(msg, msg+offset, end);
        return p+2;
    }
    else{

        int len = p[0];
        p+=1;

        if(p + len + 1 > end){
            LOG_ERR("end of message");
            exit(EXIT_FAILURE);
        }
      
        LOG_INFO("%.*s",len,p);
        p+=len;

        if(p[0]){
            LOG_INFO(".");
            print_name(msg, p, end);
        }
        else{
            return p+1;
        }

    }


}