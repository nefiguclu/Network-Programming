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


#define MAX_INPUT 512
#define MAX_RESPONSE 1024



void get_input(char *promt, char *buffer);
void send_format(int sockfd, char *text,...);
int parse_response(char *response);
void wait_response(int sockfd,int expecting);
int connect_to_host(char *hostname,char *port);


int main(){

    fprintf(stderr,"\n\nusage: cat input | ./smtp_client \n\n");


    char hostname[MAX_INPUT];
    get_input("Mail Server: ",hostname);

    printf("connecting to mail server %s\n",hostname);
    int serverfd=connect_to_host(hostname, "25");

    wait_response(serverfd,220);

    send_format(serverfd, "HELO TESTAGENT\r\n");
    wait_response(serverfd,250);

    char send_addr[MAX_INPUT];
    get_input("From: ",send_addr);
    send_format(serverfd, "MAIL FROM:<%s>\r\n",send_addr);
    wait_response(serverfd,250);


    char recp_addr[MAX_INPUT];
    get_input("To: ",recp_addr);
    send_format(serverfd, "RCPT TO:<%s>\r\n",recp_addr);
    wait_response(serverfd,250);

    send_format(serverfd, "DATA\r\n");
    wait_response(serverfd,354);

    char subject[MAX_INPUT];
    get_input("Subject: ",subject);
    send_format(serverfd, "From:<%s>\r\n",send_addr);
    send_format(serverfd, "To:<%s>\r\n",recp_addr);
    send_format(serverfd, "Subject:<%s>\r\n",subject);


    send_format(serverfd, "\r\n");


    printf("--Enter E-mail body end with single \".\" line\n");

    while(1){
        char body[MAX_INPUT];
        get_input("> ",body);
        send_format(serverfd, "%s\r\n",body);
        if(!strcmp(body,".")){
            break;
        }
    }

    wait_response(serverfd,250);

    send_format(serverfd, "QUIT \r\n");
    wait_response(serverfd,221);

    printf("Closing Socket .\n");
    close(serverfd);

    return 0;
}


void get_input(char *promt, char *buffer){

    printf("%s\n", promt);

    fgets(buffer,MAX_INPUT,stdin);
    int read=strlen(buffer);
    if(read > 0){
        buffer[read-1]='\0'; //remove new line character
    }
}


void send_format(int sockfd, char *text,...){

    char buf[MAX_INPUT];
    va_list arg;
    va_start(arg, text);

    vsprintf(buf,text,arg);
    va_end(arg);

    send(sockfd,buf,strlen(buf),0);

    printf("Client Req: %s\n",buf);

}


void wait_response(int sockfd,int expecting){

    char response_buf[MAX_RESPONSE+1];
    int code=0;

    char *p=response_buf;
    char *end=response_buf + MAX_RESPONSE;

    do{
        if(p==end){
            fprintf(stderr,"buffer overflow\n");
            exit(EXIT_FAILURE);
        }

        int bytes_received = recv(sockfd,response_buf,end-p,0);
        if(bytes_received<1){
            fprintf(stderr,"connection closed by server\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        p+=bytes_received;
        *p= '\0';

        code=parse_response(response_buf);

    }while(code == 0);


    if(code != expecting){
        fprintf(stderr,"Error wrong code\n");
        fprintf(stderr,"Response %s\n",response_buf);
        exit(EXIT_FAILURE);
    }

    printf("Server Response: %s\n",response_buf);

}


int parse_response(char *response){

    char *p = response;

    if( !p[0] || !p[1] || !p[2] )
        return 0;

    for( ; p[3]; p++){
        if( p==response || p[-1]=='\n'){
            if(isdigit(p[0]) && isdigit(p[1]) && isdigit(p[2])){
                if(p[3] != '-'){
                    if(strstr(p,"\r\n")){
                        return strtol(p,NULL,10);
                    }
                }
            }
        }
    }

    return 0;
}


int connect_to_host(char *hostname,char *port){

    struct addrinfo hints;
    struct addrinfo *peer_address;

    memset(&hints,0,sizeof(hints));
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags= AI_ALL | AI_ADDRCONFIG;

    printf("Configuring remote adress...\n");
    if(getaddrinfo(hostname, port, &hints, &peer_address)){
        fprintf(stderr,"Error getaddrinfo\n %d %s \n",errno,strerror(errno));
        exit(EXIT_FAILURE);   
    }
    

    printf("Remote adress is: ");
    char adress_buf[100];
    char service_buf[100];
    getnameinfo(peer_address->ai_addr,peer_address->ai_addrlen,adress_buf,sizeof(adress_buf),service_buf,sizeof(service_buf),NI_NUMERICHOST);
    printf("%s %s \n",adress_buf,service_buf);

    printf("Creating socket...\n");
    int sockfd=socket(peer_address->ai_family,peer_address->ai_socktype,peer_address->ai_protocol);
    if(sockfd==-1){
        fprintf(stderr,"socket failed with error %d\n",errno);
        exit(EXIT_FAILURE);   
    }

    printf("Connecting...\n");
    if(connect(sockfd,peer_address->ai_addr,peer_address->ai_addrlen)){
        fprintf(stderr,"connect failed with error %d\n",errno);
        exit(EXIT_FAILURE);   
    }

    freeaddrinfo(peer_address);
    printf("Connected...\n");

    return sockfd;
}
