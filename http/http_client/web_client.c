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

#define TIMEOUT 5
#define RESPONSE_SIZE 901920


int parse_url(char *url, char **hostname, char **port, char **path);
int connect_to_host(char *hostname,char *port);
void send_request(int sockfd, char *hostname, char *port, char *path);

int main(int argc, char *argv[]){


    if(argc<2){
        fprintf(stderr,"usage: web_client url\n");
        return -1;
    }

    //char url[1024]="http://www.examle.org:8080/somepath#hash";
    char *url=argv[1];

    char *hostname;
    char *port;
    char *path;

    parse_url(url,&hostname,&port,&path);
    int sockfd=connect_to_host(hostname,port);
    send_request(sockfd,hostname,port,path);


    //abort request after a certein of time 
    clock_t start_time=clock();


    char buf[RESPONSE_SIZE+1];

    char *p=NULL;
    char *end=NULL;

    char *q=NULL;
    char *body=NULL;

    enum{content_size,chunk,connection};
    int response_type;
    int remaining;

    p=buf;
    end=p+RESPONSE_SIZE;

    while(1){

        if((clock() - start_time) / CLOCKS_PER_SEC > TIMEOUT){
            fprintf(stderr,"TIMEOUT\n");
            return -1;
        }
        if(p == end){
            fprintf(stderr,"out of buffer\n");
            return -1;
        }

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(sockfd,&reads);

        struct timeval time;
        time.tv_sec=0;
        time.tv_usec=200000;

        if((select(sockfd+1,&reads,NULL,NULL,&time)<0)){
            fprintf(stderr,"select failed\n");
            return -1;
        }


        if(FD_ISSET(sockfd, &reads)){
            int bytes_received = recv( sockfd , p, end - p , 0);

            if(bytes_received <= 0){
                if( body && response_type == connection)
                    printf("Message Body: %.*s\n",(int)(end - body) , body);

                fprintf(stderr,"Connection Closed by server\n");
                break;
            }

            //printf("\n\n Message: \n\n%.*s\n\n",bytes_received,p);

            p+=bytes_received;
            *p='\0';

            if( !body && (body = strstr(buf, "\r\n\r\n"))){
                *body='\0';
                body+=4;

                printf("\nParsing Header...\n");
                printf("\n%s\n",buf);

                if((q=strstr(buf,"Content-Length: "))){
                    q=strchr(q,' ');
                    remaining=strtol(q+1,NULL,10);
                    response_type=content_size;
                }
                else{
                    q=strstr(buf,"chunked");
                    if(q){
                        response_type=chunk;
                        remaining=0;
                    }

                    else{
                        response_type=connection;
                    }
                }
            }

            if(body){
                printf("\nParsing Body...\n");
    
                if(response_type==content_size){
                    if(p-body >= remaining){
                        printf("%.*s",remaining,body);
                        break;
                    }
                }

                else if(response_type==chunk){
                    do{
                        if(remaining==0){
                            q=strstr(body,"\r\n");
                            if(q){
                                remaining=strtol(body,NULL,16);
                                printf("\nremaining %d\n",remaining);
                                if(!remaining){
                                    goto finish;
                                }
                                body=q+2;
                            }
                            else{
                                break;
                            }
                        }
                        if( remaining && p - body >= remaining){
                            printf("%.*s",remaining,body);
                            body += remaining+2;
                            remaining=0;
                        }
                        }while(!remaining);
                }

            } //body

        } //fd_set

    } //while

finish:

    printf("\nClosing Socket...\n");
    close(sockfd);

    printf("Finished\n");

    return 0;
}



int connect_to_host(char *hostname,char *port){

    struct addrinfo hints;
    struct addrinfo *peer_addr;
    int sock;

    memset(&hints,0,sizeof(hints));
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_ALL | AI_ADDRCONFIG;

    printf("Configuring remote adress...\n");
    if(getaddrinfo(hostname,port,&hints,&peer_addr)){
        fprintf(stderr,"getaddrinfo failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Remote adress is:\n");
    char name[1024];
    char serv[1024];
    getnameinfo(peer_addr->ai_addr,peer_addr->ai_addrlen,name,sizeof(name),serv,sizeof(serv),NI_NUMERICHOST | NI_NUMERICSERV);
    printf("Name: %s\nServ: %s\n",name,serv);


    printf("Creating Socket...\n");
    sock=socket(peer_addr->ai_family,peer_addr->ai_socktype,peer_addr->ai_protocol);
    if(sock<0){
        fprintf(stderr,"socket failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Connecting...\n");
    if(connect(sock,peer_addr->ai_addr,peer_addr->ai_addrlen)){
        fprintf(stderr,"connect failed\n");
        exit(EXIT_FAILURE);        
    }


    printf("Connected...\n");
    freeaddrinfo(peer_addr);

    return sock;
}


void send_request(int sockfd, char *hostname, char *port, char *path){
    
    char buffer[2048];


    sprintf(buffer,"GET /%s HTTP/1.1\r\n",path);
    sprintf(buffer + strlen(buffer),"Host: %s:%s\r\n",hostname,port);
    sprintf(buffer + strlen(buffer),"Connection: close\r\n");
    sprintf(buffer + strlen(buffer),"\r\n");

    send(sockfd,buffer,strlen(buffer),0);

    printf("Send header:\n%s",buffer);


}

//http://www.examle.org:80/some/path/#hash
int parse_url(char *url, char **hostname, char **port, char **path){
    char *p=strstr(url,"://");
    if(p){
        *p='\0';
        p=p+3;
        if(strcmp(url,"http")){
            fprintf(stderr,"protocol must be http\n");
            return -1;
        }
    }
    else{
        p=url;
    }

    *hostname=p;

    while(*p && (*p != ':') && (*p != '/') && (*p != '#'))
        p++;
    
    *port="80"; //default port if : does not exist
    if(*p==':'){
        *p='\0';
        p++;
        *port=p;
    }
    
    while(*p && (*p != '/') && (*p != '#'))
        p++;

    *path="";

    if(*p=='/'){
        *p='\0';
        p++;
        *path=p;
     }

    while(*p && (*p != '#'))
        p++;

    if(*p=='#'){
        *p='\0';
     }

    printf("Hostname: %s\nPort: %s\nPath: %s\n",*hostname,*port,*path);
    return 0;

}